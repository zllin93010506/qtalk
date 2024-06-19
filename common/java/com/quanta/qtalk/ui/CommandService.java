package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.format.Time;
import android.widget.Toast;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.FileUtility;
import com.quanta.qtalk.provision.PackageChecker;
import com.quanta.qtalk.provision.PackageVersionUtility;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.history.HistoryDataBaseUtility;
import com.quanta.qtalk.ui.history.HistoryManager;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PtmsData;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class CommandService extends Service {
    private final static String DEBUGTAG = "QSE_CommandService";

    private static Context mContext;

    public final static String TYPE_NURSESTATION = "STATION";
    public final static String TYPE_WIFIPHONE = "PHONE";
    public final static String TYPE_PATIENT = "PATIENT";

    public final static String CAMERA_LOCAL_OPEN = "OPEN";
    public final static String CAMERA_LOCAL_CLOSE = "CLOSE";
    public final static String CAMERA_LOCAL_DEFAULT = "DEFAULT";

    public final static int command_login = 0;
    public final static int command_logout = 1;
    public final static int command_makecall = 2;
    public final static int command_nursecall = 3;
    public final static int command_handsetpickup = 4;
    public final static int command_handsethangup = 5;
    public final static int command_ttslog = 6;
    public final static int command_playsound = 7;
    public final static int command_logonstate = 8;
    public final static int command_notification = 9;
    public final static int command_sleep = 10;
    public final static int command_wake = 11;
    public final static int command_rsyslogconfig = 12;
    public final static int command_alarm = 13;
    public final static int command_resourcechanged = 14;
    public final static int command_test = 100;
    // Activate
    private final static int LOGIN_FAILURE = 0;
    private final static int LOGIN_SUCCESS = 1;
    private final static int DEACTIVATION_SUCCESS = 6;
    // Provision
    private final static int PHONEBOOK_UPDATE_SUCCESS = 2;
    private final static int PROVISION_UPDATE_SUCCESS = 1;
    private final static int LOGIN_PROVISION_SUCCESS = 3;
    private final static int LOGIN_PHONEBOOK_SUCCESS = 4;
    private final static int FAIL = 0;

    private final static int LOGIN_CB_OK = -1;
    private final static int LOGIN_CB_IDFAIL = 0;
    private final static int LOGIN_CB_SIPFAIL = 1;
    private final static int LOGIN_CB_BUSY = 2;
    private final static int LOGIN_CB_INFONULL = 3;
    private final static int LOGIN_CB_NETWORK = 4;

    private final static int STATUS_IDEL = 0;
    private final static int STATUS_SESSION = 1;
    private final static int STATUS_INCOMING = 2;
    private final static int STATUS_OUTGOING = 3;

    final static String ACCEPT_CALL = "com.quanta.qtalk.accept.call";
    final static String ACCEPT_VIDEOCALL = "com.quanta.qtalk.accept.videocall";
    final static String HANGUP_CALL = "com.quanta.qtalk.hangup.call";

    private static String versionNumber = null;
    public static QtalkSettings qtalk_settings = null;
    private static QtalkDB qtalkdb;

    private static String phonebook_uri = null;
    private static String provision_uri = null;

    private static QThProvisionUtility qtnProvisionMessenger;

    private static String session = "000000000000";

    private final static Object mLogoutLock = new Object();

    private static boolean isDeactivate = false;

    private static String logout_uri = null;
    private static String provisionID = null;
    private static String userID = null;
    private static String provisionType = null;

    private static Bundle msgData = null;

    // nurse sequence call
    private static NurseCallThread mNurseCallThread = null;
    private static int NurseCallThreadHashCode = 0;
    public static boolean shouldNextNurseCall = false;
    public static String mCallID = "";
    public static String mRemoteID = "";
    public static boolean lastcall = false;

    private static Handler rsyslogHandler = new Handler();
    private static Runnable rsyslogRun = null;

    private static Handler mobileSleepHandler = new Handler(Looper.getMainLooper());
    private static Runnable mobileSleepRun = null;

    @SuppressLint("HandlerLeak")
    private Handler mLogoutHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {

            }
        }
    };
    @SuppressLint("HandlerLeak")
    private Handler mActivateHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case LOGIN_FAILURE:
                    Log.d(DEBUGTAG, " LOGIN_FAILED");
                    if (msg.getData().getString("msg") != null) {
                        if (msg.getData().getString("msg").contentEquals("confirmfail")
                                || msg.getData().getString("msg").contentEquals("idfail")) {
                            sendLoginCBBroadcast(LOGIN_CB_IDFAIL);
                        } else {
                            sendLoginCBBroadcast(LOGIN_CB_BUSY);
                        }
                    } else {
                        Log.d(DEBUGTAG, "network fail");
                        sendLoginCBBroadcast(LOGIN_CB_NETWORK);
                    }
                    break;
                case LOGIN_SUCCESS:
                    Log.d(DEBUGTAG, " LOGIN_SUCCESS");
                    Flag.SIPRunnable.setState(false);
                    Flag.SessionKeyExist.setState(true);
                    Provision(msg.getData());
                    break;

                case DEACTIVATION_SUCCESS:
                    Log.d(DEBUGTAG, "The app is deactivation!!!");
                    break;

            }
        }

    };

    private Handler mProvisionHandler = new Handler(Looper.getMainLooper()) {
        public void handleMessage(final Message msg) {
            switch (msg.what) {
                default:
                    break;
                case PHONEBOOK_UPDATE_SUCCESS:
                    Log.d(DEBUGTAG, "PHONEBOOK_UPDATE_SUCCESS");
                    postPhonebook(msg.getData());
                    break;
                case PROVISION_UPDATE_SUCCESS:
                    Log.d(DEBUGTAG, "PROVISION_UPDATE_SUCCESS");
                    msgData = msg.getData();
                    new Thread(PhonebookRunnable).start();
                    break;
                case LOGIN_PROVISION_SUCCESS:
                    Log.d(DEBUGTAG, "LOGIN_PROVISION_SUCCESS");
                    break;
                case LOGIN_PHONEBOOK_SUCCESS:
                    Log.d(DEBUGTAG, "LOGIN_PHONEBOOK_SUCCESS");
                    break;
                case DEACTIVATION_SUCCESS:
                    Log.d(DEBUGTAG, "DEACTIVATION_SUCCESS");
                    break;
                case FAIL:
                    sendLoginCBBroadcast(LOGIN_CB_BUSY);
                    Log.d(DEBUGTAG, "FAIL");
                    break;

            }
        }
    };

    private Handler mSipHandler = new Handler() {
        public void handleMessage(final Message msg) {
            switch (msg.what) {
                default:
                    break;
                case LOGIN_SUCCESS:
                    Log.d(DEBUGTAG, "SIP LOGIN SUCCESS");
                    LogonStateReceiver.notifyLogonState(getApplicationContext(), true, null, "QSPT-VC");
                    sendLoginCBBroadcast(LOGIN_CB_OK);
                    break;
                case LOGIN_FAILURE:
                    Log.d(DEBUGTAG, "SIP LOGIN FAILURE");
                    sendLoginCBBroadcast(LOGIN_CB_SIPFAIL);
                    break;
            }
        }

    };
    Runnable PhonebookRunnable = new Runnable() {
        @Override
        public void run() {
            Phonebook(msgData);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        Hack.init(this);
        PermissionRequestActivity.requestPermission(this);
        Hack.getStoragePath();
        Hack.checkLogFolder(this);
        Log.d(DEBUGTAG, "==> onCreate process id:" + Process.myPid());

        RingingUtil RU;
        RU = new RingingUtil();
        RU.init(this);
        mContext = this;

        final Intent myintent = new Intent();
        myintent.setClass(CommandService.this, QTReceiver.class);
        myintent.setClass(CommandService.this, LauncherReceiver.class);
        myintent.setAction(QtalkAction.ACTION_APP_START);
        sendBroadcast(myintent);

        String string = PackageVersionUtility.getVersion(this);
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, string);

        String provision = Hack.getStoragePath() + "provision.xml";
        File dirFile = new File(provision);
        QtalkEngine my_engine = QTReceiver.engine(this, false);

        boolean flag = true;

        Log.d(DEBUGTAG,
                "my_engine:" + my_engine + ", my_engine.isLogon():" + my_engine.isLogon()
                        + ", Flag.Activation.getState():" + Flag.Activation.getState() + ", dirFile.exists():"
                        + dirFile.exists() + ", flag=" + flag);
        if (flag && ((my_engine != null && my_engine.isLogon()) || (dirFile.exists()))) {
            if ((dirFile.exists())) {
                startForegroundService(new Intent(this, QTService.class));
            }
        }

		if(Hack.isPhone()) {
			Intent loginintent = new Intent();
			loginintent.setPackage("com.quanta.qoca.spt.nursingapp");
			loginintent.setAction(LauncherReceiver.ACTION_VC_GET_LOGIN_DATA);
			this.sendBroadcast(loginintent);
		}
		Log.d(DEBUGTAG,"ACTION_VC_GET_LOGIN_DATA");
		Hack.set_alarm(this, LauncherReceiver.class, LauncherReceiver.ACTION_VC_ALARM, 60);
    }

    private void startForegroundNotification() {
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
                .setContentTitle("CommandService")
                .setGroup("qtalk_server")
                .setPriority(Notification.PRIORITY_LOW)
                .setContentText("").build();
        startForeground(2222, notification);
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    private void _doMobile(final String action) {
        Log.d(DEBUGTAG, "mobile action:" + action);
        try {
            if (qtalk_settings == null)
                qtalk_settings = getSetting(false);
        } catch (FailedOperateException e) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "doMobile", e);
        }
        if (QThProvisionUtility.provision_info == null) {
            Log.d(DEBUGTAG, "provision_info == null, try to recover");
            QtalkEngine mEngine = QTReceiver.engine(this, false);
            try {
                mEngine.login(CommandService.this);
            } catch (FailedOperateException e) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e);
            }
        }

        if (TextUtils.equals(action, "wake") && TextUtils.equals(QtalkSettings.tts_deviceId.trim(), "")) {
            synchronized (mLogoutLock) {
                String path = Hack.getTTSFile();
                File file = new File(path);
                if (file.exists()) {
                    try {
                        BufferedReader br = new BufferedReader(new FileReader(path));
                        String ptmsIp = br.readLine();
                        String deviceId = br.readLine();
                        Hack.setPTMSInfo(ptmsIp, deviceId);
                        br.close();
                    } catch (Exception e) {
                        Log.e(DEBUGTAG, "", e);
                    }
                }
            }
        }

        if (QThProvisionUtility.provision_info == null) {
            Log.d(DEBUGTAG, "provision_info == null, skip do mobile:" + action);
        } else {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
                        qtnMessenger.mobile(CommandService.this, QThProvisionUtility.provision_info.authentication_id,
                                action, QThProvisionUtility.provision_info.sip_id, QtalkSettings.tts_deviceId);
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        Log.e(DEBUGTAG, "", e);
                    }
                }
            }).start();
        }
    }

    private void doMobile(final String action) {
        if (Hack.isPhone())
            if ((Flag.Activation.getState() && Flag.SessionKeyExist.getState()) || TextUtils.equals(action, QThProvisionUtility.MOBILE_ACTION_CLEAR))
                _doMobile(action);
            else
                Log.d(DEBUGTAG, "vc isn,t login, skip " + action);
    }

    private void setVCLOGIN(boolean status) {
        Log.i(DEBUGTAG, "set VCLOGIN=" + status);
        PreferenceManager.getDefaultSharedPreferences(CommandService.this).edit().putBoolean("VCLOGIN", status)
                .commit();
    }

    private boolean getVCLOGIN() {
        boolean ret = PreferenceManager.getDefaultSharedPreferences(CommandService.this).getBoolean("VCLOGIN", false);
        Log.i(DEBUGTAG, "VCLOGIN=" + ret);
        return ret;
    }

    public void handleTTS(String ptms, String device) {
        boolean isChanged = false;
        Log.d(DEBUGTAG,
                "old ptms server = " + QtalkSettings.ptmsServer + " old deviceid = " + QtalkSettings.tts_deviceId);
        if (!TextUtils.equals(device, "unknown")) {
            if (!TextUtils.equals(QtalkSettings.tts_deviceId, device)) {
                Log.d(DEBUGTAG, "device id is changed from " + QtalkSettings.tts_deviceId + " to " + device);
                isChanged = true;
            }
        }
        if (!TextUtils.equals(ptms, "")) {
            if (!TextUtils.equals(QtalkSettings.ptmsServer, ptms)) {
                Log.d(DEBUGTAG, "ptms server is changed from " + QtalkSettings.ptmsServer + " to " + ptms);
                isChanged = true;
            }
        }

        if (isChanged) {
            Hack.setPTMSInfo(ptms, device);
            synchronized (mLogoutLock) {
                String ttspath = Hack.getTTSFile();
                File ttsfile = new File(ttspath);
                File dirFile = new File(Hack.getStoragePath());
                if (ttsfile.exists()) {
                    try {
                        ttsfile.delete();
                    } catch (Exception e) {
                        Log.e(DEBUGTAG, "delete ttsInfo error", e);
                    }
                }
                if (!dirFile.exists())
                    dirFile.mkdir();
                FileWriter FW = null;
                try {
                    FW = new FileWriter(ttsfile);
                    FW.write(ptms);
                    FW.write('\n');
                    FW.write(device);
                    FW.write('\n');
                    FW.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG, "Writing ttslist error!!", e);
                }
            }
            synchronized (CommandService.this) {
                if (rsyslogRun != null) {
                    rsyslogHandler.removeCallbacks(rsyslogRun);
                    rsyslogRun = null;
                }
                rsyslogRun = new Runnable() {
                    @Override
                    public void run() {
                        Log.setSysLogConfigInLogback(QtalkSettings.ptmsServer, QtalkSettings.tts_deviceId);
                    }
                };
                rsyslogHandler.postDelayed(rsyslogRun, 6000);
            }
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags | Service.START_FLAG_REDELIVERY, startId);
        int command = -1;
        Bundle bundle = null;
        mContext = this;
        if (intent != null)
            bundle = intent.getExtras();
        boolean CameraLocalDefault = false;
        if (bundle != null) {
            command = bundle.getInt("KEY_COMMAND", -1);
            Log.d(DEBUGTAG, "command=" + command);
        } else
            Log.e(DEBUGTAG, "no bondle");
        if (command != -1) {
            int appstatus = -1;
            String AppType = null;
            switch (command) {
                case command_login:
                    Log.d(DEBUGTAG, "command_login, start SIPRunnable=" + Flag.SIPRunnable.getState()
                            + ", SessionKeyExist=" + Flag.SessionKeyExist.getState()
                            + ", Activation=" + Flag.Activation.getState());
                    String Uid = null;
                    String Psw = null;
                    String Ipaddr = null;
                    String Type = null;
                    String CameraSwitch = null;
                    String DeviceId = null;
                    String Ptmsip = null;
                    boolean Control_keep_visible = false;
                    if (bundle != null) {
                        Uid = bundle.getString("KEY_LOGIN_UID");
                        Psw = bundle.getString("KEY_LOGIN_PSWD");
                        Ipaddr = bundle.getString("KEY_LOGIN_IPADDR");
                        Type = bundle.getString("KEY_LOGIN_TYPE");
                        DeviceId = bundle.getString("KEY_DEVICE_ID", "unknown");
                        Ptmsip = bundle.getString("KEY_PTMS_IP", "");
                        Control_keep_visible = bundle.getBoolean("KEY_LOGIN_CONTROL", false);
                        CameraSwitch = bundle.getString("KEY_LOGIN_CAM_DEF", CAMERA_LOCAL_DEFAULT);
                        QtalkSettings.type = Type;
                        Log.d(DEBUGTAG, "Receive Login UID :" + Uid + " ,PSW :" + Psw + " Ipaddr :" + Ipaddr + " Type:"
                                + Type + " DeviceId:" + DeviceId + " Control visible:" + Control_keep_visible
                                + " CameraSwitch:" + CameraSwitch);

                        handleTTS(Ptmsip, DeviceId);
                        // if(bundle.getBoolean("KEY_MOBILE", false)) {
                        doMobile(QThProvisionUtility.MOBILE_ACTION_WAKE);
                    }
                    if (bundle == null || Uid == null || Ipaddr == null || Type == null) {
                        Log.d(DEBUGTAG, "LOGIN INFORMATION ERROR");
                        sendLoginCBBroadcast(LOGIN_CB_INFONULL);
                    } else {
                        if (Type.contentEquals(TYPE_NURSESTATION)) {
                            AppType = "?type=ns";
                        } else if (Type.contentEquals(TYPE_WIFIPHONE)) {
                            AppType = "?type=qtfa";
                        } else if (Type.contentEquals(TYPE_PATIENT)) {
                            AppType = "?type=hh";
                        } else
                            ;

                        if (CameraSwitch.contentEquals(CAMERA_LOCAL_OPEN)) {
                            CameraLocalDefault = true;
                        } else if (CameraSwitch.contentEquals(CAMERA_LOCAL_CLOSE)) {
                            CameraLocalDefault = false;
                        } else {
                            if (Type.contentEquals(TYPE_NURSESTATION) || Type.contentEquals(TYPE_WIFIPHONE)) {
                                CameraLocalDefault = true;
                            } else {
                                CameraLocalDefault = false;
                            }
                        }

                        if ((!getVCLOGIN()) ||
                                (!Flag.Activation.getState()) ||
                                (!Flag.SessionKeyExist.getState())) {
                            Login(Uid, Psw, Ipaddr, AppType, DeviceId, Control_keep_visible, CameraLocalDefault);
                            setVCLOGIN(true);
                        } else if (!Flag.SIPRunnable.getState()) {
                            Flag.SIPRunnable.setState(true);
                            QTReceiver.engine(CommandService.this, true);
                            sendLoginCBBroadcast(LOGIN_CB_OK);
                        } else {
                            Log.d(DEBUGTAG, "<warning> skip Login command, because now is in Login progress");
                            sendLoginCBBroadcast(LOGIN_CB_OK);
                        }
                    }
                    break;
                case command_logout:
                    Bundle logout_bundle = intent.getExtras();
                    String login_after = logout_bundle.getString("KEY_LOGIN_AFTER");
                    Log.d(DEBUGTAG, "command_logout, start SIPRunnable=" + Flag.SIPRunnable.getState()
                            + ", SessionKeyExist=" + Flag.SessionKeyExist.getState()
                            + ", Activation=" + Flag.Activation.getState()
                            + ", KEY_LOGIN_AFTER=" + login_after);

                    if (logout_bundle.getBoolean("KEY_START_ACTIVITY") && Flag.SessionKeyExist.getState()
                            && Flag.Activation.getState()) {
                        Log.d(DEBUGTAG, "command_logout do notthing.");
                        sendLoginCBBroadcast(LOGIN_CB_OK);
                    } else if (!TextUtils.isEmpty(login_after)) {
                        Logout(login_after);
                    } else {
                        PtmsData.clearData();
                        Log.d(DEBUGTAG, "command_logout do logout without KEY_LOGIN_AFTER");
                        Logout("No");
                    }
                    break;
                case command_makecall:
                    Log.d(DEBUGTAG, "command_makecall, start");
                    Bundle makecall_bundle = intent.getExtras();
                    if (makecall_bundle != null) {
                        QtalkSettings.uidCall = makecall_bundle.getString("KEY_MAKECALL_UID");
                        if (QtalkSettings.uidCall != null) {
                            make_uid_call();
                        }
                    } else {
                        Log.d(DEBUGTAG, "makecall null");
                    }
                    break;
                case command_nursecall:
                    Log.d(DEBUGTAG, "command_nursecall, start");
                    Bundle nursecall_bundle = intent.getExtras();
                    if (mNurseCallThread == null) {
                        if (nursecall_bundle != null) {
                            QtalkSettings.nurseCall_uid = nursecall_bundle.getString("KEY_NURSE_UID");
                            Log.d(DEBUGTAG, "KEY_NURSE_UID: " + QtalkSettings.nurseCall_uid);
                            if (QtalkSettings.nurseCall_uid != null) {
                                QtalkSettings.nurseCall = true;
                                NurseSeqCall();
                            }
                        } else {
                            Log.d(DEBUGTAG, "<warning> command_nursecall: null");
                        }
                    } else {
                        Log.d(DEBUGTAG, "<warning> command_nursecall: Thread is running");
                    }
                    break;
                case command_handsetpickup:
                    Log.d(DEBUGTAG, "command_handsetpickup, start");
                    appstatus = AppStatus.getStatus();
                    switch (appstatus) {
                        case STATUS_IDEL:
                            LauchApp();
                            break;
                        case STATUS_SESSION:
                            break;
                        case STATUS_INCOMING:
                            AcceptCall();
                            break;
                        case STATUS_OUTGOING:
                            break;
                        default:
                            break;
                    }
                    QtalkSettings.nurseCall = false;
                    break;
                case command_handsethangup:
                    Log.d(DEBUGTAG, "command_handsethangup, start");
                    appstatus = AppStatus.getStatus();
                    Log.d(DEBUGTAG, "command_handsethangup, appstatus=" + appstatus);
                    switch (appstatus) {
                        case STATUS_IDEL:
                            break;
                        case STATUS_SESSION:
                            HangupCall();
                            break;
                        case STATUS_INCOMING:
                            HangupCall();
                            break;
                        case STATUS_OUTGOING:
                            HangupCall();
                            break;
                        default:
                            break;
                    }
                    QtalkSettings.nurseCall = false;
                    break;
                case command_ttslog:
                    Log.d(DEBUGTAG, "command_ttslog, start");
                    writeTTSLog(intent.getExtras());
                    break;
                case command_test:
                    Log.d(DEBUGTAG, "command_test, start");
                    Bundle test_bundle = intent.getExtras();
                    Log.d(DEBUGTAG, "command_test, " + test_bundle.getString("source", "unknown"));
                    break;
                case command_playsound:
                    RingingUtil.playSoundIncomingCall(this);
                    break;
                case command_logonstate:
                    Intent stp_status_intent = new Intent();
                    stp_status_intent.setAction(LauncherReceiver.ACTION_VC_LOGON_STATE_CB);
                    Bundle stp_status_bundle = new Bundle();
                    stp_status_bundle.putBoolean("state", LogonStateReceiver.logon_state);
                    stp_status_bundle.putString("msg", LogonStateReceiver.logon_msg);
                    stp_status_intent.putExtras(stp_status_bundle);
                    sendBroadcast(stp_status_intent);
                    Log.d(DEBUGTAG, "command_logonstate, " + LogonStateReceiver.logon_state + ' ' + LogonStateReceiver.logon_msg);
                    break;
                case command_notification:
                    String id = bundle.getString("KEY_ID", "unknown");
                    String from = bundle.getString("KEY_FROM", "unknown");
                    String incommingString = getResources().getString(R.string.incoming);
                    Log.d(DEBUGTAG,
                            "command_notification, id=" + id + ", from=" + from + ", incoming=" + incommingString);
                    NotificationManager notificationManager = (NotificationManager) getSystemService(
                            Context.NOTIFICATION_SERVICE);
                    notificationManager.cancel(1);

                    Notification.Builder builder = new Notification.Builder(this);
                    builder.setWhen(System.currentTimeMillis());
                    builder.setSmallIcon(R.drawable.incoming);
                    builder.setTicker(incommingString);
                    builder.setContentTitle(incommingString);
                    Notification notification = builder.build();
                    notification.flags |= Notification.FLAG_SHOW_LIGHTS;
                    notification.flags |= Notification.FLAG_NO_CLEAR; // keep
                    // notification
                    // on unless
                    // logout

                    notificationManager.notify(999, notification);
                    break;
                case command_sleep:
                    if (getVCLOGIN() && !Hack.isScreenOn(CommandService.this)) {
                        Log.d(DEBUGTAG, "do mobile sleep");
                        doMobile(QThProvisionUtility.MOBILE_ACTION_SLEEP);
                        NotificationManager notificationManager1 = (NotificationManager) getSystemService(
                                Context.NOTIFICATION_SERVICE);
                        notificationManager1.cancel(1);
                        if (Flag.SIPRunnable.getState()) {
                            Flag.SIPRunnable.setState(false);
                            QTReceiver.finishQT(CommandService.this);
                        }
                    } else {
                        Log.d(DEBUGTAG, "skip mobile sleep when screen on.");
                    }
                    break;
                case command_wake:
                    mobileSleepHandler.removeCallbacksAndMessages(null);
                    if (getVCLOGIN() && Hack.isScreenOn(CommandService.this)) {
                        doMobile(QThProvisionUtility.MOBILE_ACTION_WAKE);
                        if (!Flag.SIPRunnable.getState()) {
                            Flag.SIPRunnable.setState(true);
                            QTReceiver.engine(CommandService.this, true);
                        }

                        if (QThProvisionUtility.provision_info != null) {
                            new Thread(new Runnable() {
                                @Override
                                public void run() {
                                    try {
                                        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler,
                                                null);
                                        Log.d(DEBUGTAG, "ServiceChanged command_wake uid="
                                                + QThProvisionUtility.provision_info.authentication_id);
                                        qtnMessenger.ServiceChanged(
                                                QThProvisionUtility.provision_info.authentication_id, false);
                                    } catch (Exception e) {
                                        // TODO Auto-generated catch block
                                        Log.e(DEBUGTAG, "", e);
                                    }
                                }
                            }).start();
                        } else {
                            Log.d(DEBUGTAG, "ServiceChanged command_wake provision_info=null");
                        }
                    }
                    break;
                case command_rsyslogconfig:
                    Hack.setPTMSInfo(bundle.getString("KEY_PTMSSERVER"), bundle.getString("KEY_DEVICEID"));
                    synchronized (CommandService.this) {
                        if (rsyslogRun != null) {
                            rsyslogHandler.removeCallbacks(rsyslogRun);
                            rsyslogRun = null;
                        }
                        rsyslogRun = new Runnable() {
                            @Override
                            public void run() {
                                Log.setSysLogConfigInLogback(QtalkSettings.ptmsServer, QtalkSettings.tts_deviceId);
                            }
                        };
                        rsyslogHandler.postDelayed(rsyslogRun, 6000);
                    }
                    break;
                case command_alarm:
                    Log.d(DEBUGTAG, "command_alarm");
                    break;
                case command_resourcechanged:
                    if (QThProvisionUtility.provision_info == null) {
                        Log.d(DEBUGTAG, "ServiceChanged command_resourcechanged provision_info=null");
                        break;
                    }
                    final String changedResource = intent.getStringExtra("KEY_RESOURCE_CHANGED");
                    Log.d(DEBUGTAG, "ServiceChanged command_resourcechanged uid="
                            + QThProvisionUtility.provision_info.authentication_id);
                    Log.d(DEBUGTAG, "ServiceChanged changedResource=" + changedResource);
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
                                if (Hack.isPhone() && changedResource.contains("practitioners"))
                                    qtnMessenger.ServiceChanged(QThProvisionUtility.provision_info.authentication_id,
                                            true);
                                else if (Hack.isStation() &&
                                        (changedResource.contains("departments") ||
                                                changedResource.contains("beds") ||
                                                changedResource.contains("patients") ||
                                                changedResource.contains("practitioners") ||
                                                changedResource.contains("devices"))) {
                                    qtnMessenger
                                            .ServiceChangedForNS(QThProvisionUtility.provision_info.authentication_id);
                                }
                            } catch (Exception e) {
                                // TODO Auto-generated catch block
                                Log.e(DEBUGTAG, "", e);
                            }
                        }
                    }).start();
                    break;
                default:
                    break;
            }
        } else
            Log.e(DEBUGTAG, "no KEY_COMMAND");

        //startForegroundNotification();
        return START_STICKY;
    }

    /*
     * @Override protected void onHandleIntent(Intent intent) {
     * Log.d(DEBUGTAG,"onHandleIntent"); }
     */
    @Override
    public void onDestroy() {
        Log.d(DEBUGTAG, "onDestroy");
        super.onDestroy();
    }

    // --------------TTS
    // Log----------------------------------------------------------------------
    private void writeTTSLog(Bundle bundle) {
        if (bundle != null) {
            String tts_msg = bundle.getString("TTS_BROADCAST_MSG");
            long tts_time = bundle.getLong("TTS_BROADCAST_TIME");
            String tts_from = bundle.getString("TTS_BROADCAST_FROM");
            String tts_to = bundle.getString("TTS_BROADCAST_TO");
            String tts_dir = bundle.getString("TTS_BROADCAST_DIRECTION");
            int tts_type = 1;
            if (tts_dir != null && tts_dir.contentEquals("IN")) {
                tts_type = 1;
            } else {
                tts_type = 2;
            }
            HistoryManager.addCallLog(this, null, "tts_msg", tts_msg, "", "", "tts", "tts", tts_type, 0, tts_time, "",
                    false);
        }
    }

    // --------------hand set
    // function------------------------------------------------------------
    private void LauchApp() {
        Log.d(DEBUGTAG, "Launch App");
        PackageManager mgr = mContext.getPackageManager();
        // Intent intent = mgr.getLaunchIntentForPackage("com.quanta.qtalk");
        Intent intent = new Intent(Intent.ACTION_MAIN);
        ComponentName cmp = new ComponentName("com.quanta.qtalk", "com.quanta.qtalk.QtalkMain");
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.addFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(cmp);
        mContext.startActivity(intent);
    }

    private void AcceptCall() {
        Intent intent = new Intent();
        intent.setAction(ACCEPT_CALL);
        sendBroadcast(intent);
    }

    private void HangupCall() {
        Intent intent = new Intent();
        intent.setAction(HANGUP_CALL);
        sendBroadcast(intent);
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            Log.e(DEBUGTAG, "", e);
        }
    }

    // --------------make call
    // function-----------------------------------------------------------
    private void make_uid_call() {
        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
        QtalkDB qtalkdb = new QtalkDB();

        Cursor cs = qtalkdb.returnAllPBEntry();
        int size = cs.getCount();
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "size=" + size);
        cs.moveToFirst();
        int cscnt = 0;
        for (cscnt = 0; cscnt < cs.getCount(); cscnt++) {
            if (cs.getColumnIndex("uid") != -1) {
                String Uid = QtalkDB.getString(cs, "uid");

                if (QtalkSettings.uidCall != null && Uid != null) {
                    if (Uid.equals(QtalkSettings.uidCall)) {
                        String makecall_remoteID = QtalkDB.getString(cs, "number");
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "QtalkSettings.uidCall " + QtalkSettings.uidCall + " uid: " + Uid
                                    + " number: " + makecall_remoteID);
                        makeCall(makecall_remoteID, true);
                        break;
                    }
                }
                cs.moveToNext();
            }
            if (cscnt == cs.getCount() - 1) {
                Log.d(DEBUGTAG, "CAN'T FIND THE NUMBER AT PHONEBOOK!!");
            }
        }
        QtalkSettings.uidCall = "";
    }

    protected boolean makeCall(String remoteID, boolean enableVideo) {
        boolean result = false;
        // Use call engine to Make call
        try {
            QTReceiver.engine(this, false).makeCall(null, remoteID, enableVideo);
        } catch (FailedOperateException e) {
            Log.e(DEBUGTAG, "makeCall", e);
        }
        return result;
    }

    // --------------nurse call
    // function----------------------------------------------------------
    private void NurseSeqCall() {
        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
        QtalkDB qtalkdb = new QtalkDB();

        Cursor cs = qtalkdb.returnAllPBEntry();
        int size = cs.getCount();
        if (size > 0) {
            QtalkSettings.nurseCall = true;
            mNurseCallThread = new NurseCallThread();
            mNurseCallThread.start();
        } else {
            QtalkSettings.nurseCall = false;
            Log.d(DEBUGTAG, "WARNING DB EMPTY!!");
        }
    }

    class NurseCallThread extends Thread {
        @SuppressWarnings("null")
        @Override
        public void run() {
            try {
                NurseCallThreadHashCode = this.hashCode();

                Log.d(DEBUGTAG, "=> sequence call(), hashcode: " + NurseCallThreadHashCode);

                int round = 0;
                String timestamp = String.valueOf(System.currentTimeMillis());
                String[] make_call_name = QtalkSettings.nurseCall_uid.split("::");
                String[] make_call_number = new String[make_call_name.length];

                Log.d(DEBUGTAG, "=> nurseCall_uid: " + QtalkSettings.nurseCall_uid + ", make_call_name.length: "
                        + make_call_name.length);

                // ------------------------------------------------------------------------------
                // find the number
                QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
                QtalkDB qtalkdb = new QtalkDB();

                Cursor cs = qtalkdb.returnAllPBEntry();
                int size = cs.getCount();

                // ------------------------------------------------------------------------------
                for (int i = 0; i < make_call_name.length; i++) {
                    cs.moveToFirst();
                    boolean isfound = false;
                    for (int cscnt = 0; cscnt < cs.getCount(); cscnt++) {
                        if (cs.getColumnIndex("uid") != -1) {
                            String Uid = QtalkDB.getString(cs, "uid");
                            if (make_call_name[i] != null && Uid != null) {
                                if (Uid.equals(make_call_name[i])) {
                                    isfound = true;
                                    String makecall_remoteID = QtalkDB.getString(cs, "number");
                                    make_call_number[i] = makecall_remoteID;
                                    cs.moveToFirst();
                                    if (ProvisionSetupActivity.debugMode)
                                        Log.d(DEBUGTAG,
                                                "=> make_call_name " + i + ":" + make_call_name[i] + ", uid: " + Uid
                                                        + ", number: " + make_call_number[i]);
                                    break;
                                }
                            } else {
                                Log.d(DEBUGTAG, "=> make_call_name " + i + ":" + make_call_name[i] + ", uid: " + Uid
                                        + " not found");
                            }
                            cs.moveToNext();
                        }
                        if (cscnt == cs.getCount() - 1 && !isfound) {
                            make_call_number[i] = "unknown";
                        }
                    }
                }

                int makecall_index = 0;
                // int count = QtalkSettings.timeout;
                int checkInterval = 100; // msec
                int timeoutCnt = QtalkSettings.timeout * (1000 / checkInterval);
                int count = timeoutCnt;
                boolean first = true;
                while (QtalkSettings.nurseCall && NurseCallThreadHashCode == this.hashCode()) {
                    if (make_call_number[makecall_index].contentEquals("unknown")) {
                        makecall_index++;
                        if (makecall_index >= make_call_number.length) {
                            round++;
                            makecall_index = 0;
                        }
                    }
                    if (shouldNextNurseCall || count >= timeoutCnt) {
                        if (lastcall)
                            break;

                        String displayName = make_call_name[makecall_index] + "-" + timestamp;
                        if (!first) {
                            try {
                                QTReceiver.engine(mContext, false).hangupCall(mCallID, mRemoteID, false);
                            } catch (FailedOperateException e) {
                                Log.e(DEBUGTAG, "", e);
                            }
                        } else {
                            first = false;
                        }
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "=> nurseCall running, call_number: " + make_call_number[makecall_index]
                                    + ", displayName: " + displayName);

                        makeSeqCall(make_call_number[makecall_index], true, displayName);
                        // QTReceiver.startOutgoingCallActivity(mContext, "0",
                        // make_call_number[makecall_index], false);
                        if (makecall_index == make_call_number.length - 1) {
                            makecall_index = 0;
                            round++;
                        } else {
                            makecall_index++;
                        }
                        count = 0;
                        shouldNextNurseCall = false;
                    } else {
                        count++;
                    }
                    if (round >= 1) {
                        lastcall = true;
                    }
                    try {
                        // linzl: The thread may be locked in the sleep, add log to verify it;
                        // Log.d(DEBUGTAG, "sleep start:"+checkInterval);
                        Thread.sleep(checkInterval);
                        // Log.d(DEBUGTAG, "sleep end");
                    } catch (InterruptedException e) {
                        Log.e(DEBUGTAG, "", e);
                    }
                }
                Log.d(DEBUGTAG, "QtalkSettings.nurseCall:" + QtalkSettings.nurseCall + ", NurseCallThreadHashCode:"
                        + NurseCallThreadHashCode + ", lastcall:" + lastcall);

                QtalkSettings.nurseCall_uid = "";
                mNurseCallThread = null;
                lastcall = false;
                QtalkSettings.nurseCall = false;
                Log.d(DEBUGTAG, "<= sequence call, exit");
            } catch (Exception e) {
                QtalkSettings.nurseCall_uid = "";
                mNurseCallThread = null;
                lastcall = false;
                QtalkSettings.nurseCall = false;
                Log.e(DEBUGTAG, "", e);
            }
        }
    }

    protected boolean makeSeqCall(String remoteID, boolean enableVideo, String DisplayName) {
        boolean result = false;
        // Use call engine to Make call
        try {
            QTReceiver.engine(this, false).makeSeqCall(null, remoteID, enableVideo, DisplayName);
        } catch (FailedOperateException e) {
            Log.e(DEBUGTAG, "makeCall", e);
        }
        return result;
    }

    // --------------login
    // function---------------------------------------------------------------

    private void Login(String Uid, String Psw, String Ipaddr, String Type, String DeviceId, boolean ControlKeepVisible,
                       boolean CamDefault) {
        Activate(Uid, Psw, Ipaddr, Type, DeviceId, ControlKeepVisible, CamDefault);
    }

    private void SIPLogin() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (QTReceiver.engine(CommandService.this, true).isLogon() == false) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "Prepare to SIP login");
                    Message msg = new Message();
                    Bundle bundle = new Bundle();
                    // Waiting for logon signal
                    for (int i = 0; i < 20; i++) {
                        if (QTReceiver.engine(CommandService.this, false).isLogon())
                            break;
                        try {
                            Thread.sleep(300);// 1000
                            if (i == 19)
                                if (ProvisionSetupActivity.debugMode)
                                    Log.d(DEBUGTAG, "SIP time out");
                        } catch (InterruptedException e) {
                            Log.e(DEBUGTAG, "onResume", e);
                        }
                    }

                    if (QTReceiver.engine(CommandService.this, false).isLogon()) {
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "SIP Login successfully");
                        mSipHandler.sendEmptyMessage(LOGIN_SUCCESS);
                    } else {
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "SIP Login failed");
                        bundle.putString(ProvisionSetupActivity.MESSAGE, "1網路忙碌中\r\n將在十秒後自動重試連線\r\n");
                        msg.setData(bundle);
                        msg.what = LOGIN_FAILURE;
                        mSipHandler.sendMessage(msg);
                    }
                } else {
                    mSipHandler.sendEmptyMessage(LOGIN_SUCCESS);
                }
            }
        }).start();

    }

    private void postPhonebook(Bundle bundle) {
        qtalk_settings.lastPhonebookFileTime = bundle.getString("timestamp");
        QThProvisionUtility.provision_info.lastPhonebookFileTime = bundle.getString("timestamp");
        qtalkdb.insertPTEntry(qtalk_settings.lastPhonebookFileTime, qtalk_settings.lastProvisionFileTime);
        Cursor cs = qtalkdb.returnAllATEntry();
        cs.close();
        qtalkdb.close();
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "qtnMessenger.provision_info.lastProvisionFileTime:"
                    + QThProvisionUtility.provision_info.lastProvisionFileTime);
        try {
            updateSetting(QThProvisionUtility.provision_info);
        } catch (FailedOperateException e) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "postPhonebook context=null");
            Message msg1 = new Message();
            msg1.what = FAIL;
            Bundle bundle_unconnect = new Bundle();
            bundle_unconnect.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
            msg1.setData(bundle_unconnect);
            mProvisionHandler.sendMessage(msg1);
        }

        Flag.SIPRunnable.setState(true);
        if (QTReceiver.engine(CommandService.this, false).isLogon()) {
            // myintent.setClass(CommandService.this,com.quanta.qtalk.ui.ContactsActivity.class);
        } else {
            SIPLogin();
        }

        // ------------------------------------------------------------[PT]
        Intent broadcast_intent = new Intent();
        broadcast_intent.setAction("android.intent.action.vc.phonebookupdate");
        Log.d("SEND", "Send Broadcast to phonebookupdate SUCCESS");
        sendBroadcast(broadcast_intent);
        // ------------------------------------------------------------[PT]

    }

    private void Phonebook(Bundle bundle) {
        // qtalk_settings.lastProvisionFileTime =
        // t.format("%Y%m%d%H%M%S")+"000";
        if (bundle != null) {
            qtalk_settings.lastProvisionFileTime = bundle.getString("timestamp");
            qtalk_settings.session = bundle.getString("session");
            qtalk_settings.key = bundle.getString("key");
        }
        if (QThProvisionUtility.provision_info == null) {
            Message msg1 = new Message();
            msg1.what = FAIL;
            Bundle bundle_unconnect = new Bundle();
            bundle_unconnect.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
            msg1.setData(bundle_unconnect);
            mProvisionHandler.sendMessage(msg1);
        }
        if (QThProvisionUtility.provision_info != null && bundle != null) {
            QThProvisionUtility.provision_info.lastProvisionFileTime = bundle.getString("timestamp");
            QThProvisionUtility.provision_info.session = bundle.getString("session");
            qtalk_settings.provisionServer = QThProvisionUtility.provision_info.provision_uri;

            if (qtalk_settings.lastPhonebookFileTime != null && qtalk_settings.lastProvisionFileTime != null)
                qtalkdb.insertPTEntry(qtalk_settings.lastPhonebookFileTime, qtalk_settings.lastProvisionFileTime);
            // qtalkdb.insertATEntry(qtalk_settings.session,qtalk_settings.key,qtnMessenger.provision_info.provision_uri,qtalk_settings.provisionID,versionNumber);
            Cursor cs1 = qtalkdb.returnAllATEntry();
            int size = cs1.getCount();
            if (ProvisionSetupActivity.debugMode)
                Log.d(DEBUGTAG, "1 DB size=" + size);

            if (size > 0) {
                cs1.moveToFirst();
                Log.d(DEBUGTAG, "release_test Flag.Activation= " + Flag.Activation.getState() + "line="
                        + Thread.currentThread().getStackTrace()[2].getLineNumber());
                Flag.Activation.setState(true);
                int cscnt = 0;
                for (cscnt = 0; cscnt < cs1.getCount(); cscnt++) {
                    qtalk_settings.session = QtalkDB.getString(cs1, "Session");
                    qtalk_settings.key = QtalkDB.getString(cs1, "Key");
                    cs1.moveToNext();
                }
            }
            cs1.close();
        }
        try {
            updateSetting(QThProvisionUtility.provision_info);
        } catch (FailedOperateException e) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "postPhonebook context=null");
        }
        PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "SIPID:" + QThProvisionUtility.provision_info.authentication_id + "       SIPPW:"
                    + QThProvisionUtility.provision_info.authentication_password);
        doMobile(QThProvisionUtility.MOBILE_ACTION_WAKE);
        phonebook_uri = QThProvisionUtility.provision_info.phonebook_uri +
                // String phonebook_uri =
                // PackageChecker.getProvisionInfo().phonebook_uri +
                // "/~kakasi/provision/provision.php"+
                qtalk_settings.provisionType + "&service=phonebook&action=check&uid=" + qtalk_settings.provisionID
                + "&session=" + qtalk_settings.session + "&timestamp=" + qtalk_settings.lastPhonebookFileTime;
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "phonebook_uri:" + phonebook_uri);
        qtnProvisionMessenger = new QThProvisionUtility(mProvisionHandler, null);
        try {
            // if( phonebook_uri.startsWith("https"))
            {
                qtnProvisionMessenger.httpsConnect(phonebook_uri, session, qtalk_settings.lastPhonebookFileTime,
                        qtalk_settings.provisionID, qtalk_settings.provisionType,
                        QThProvisionUtility.provision_info.phonebook_uri, qtalk_settings.key);
            }

        } catch (IOException e1) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "", e1);
            Message msg1 = new Message();
            msg1.what = FAIL;
            Bundle bundle_unconnect = new Bundle();
            bundle_unconnect.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
            msg1.setData(bundle_unconnect);
            mProvisionHandler.sendMessage(msg1);
        }
        // qtalkdb.close();
    }

    private void Provision(Bundle bundle) {

        if (bundle != null) {
            session = bundle.getString("session");
        }
        try {
            if (ProvisionSetupActivity.debugMode)
                Log.d(DEBUGTAG, "Reprovision Flag is " + Flag.Reprovision.getState());
            qtalk_settings = getSetting(false);
            Log.d(DEBUGTAG, "provisionServer 1:" + qtalk_settings.provisionServer);
        } catch (FailedOperateException e) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "onCreate", e);
        }
        // if
        // (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
        // {
        synchronized (mLogoutLock) {
            File dirFile = new File(Hack.getStoragePath());
            Log.d(DEBUGTAG, "dirFile.exists(): " + dirFile.exists());
            if (!dirFile.exists()) {
                dirFile.mkdir();
            }
            Flag.SIPRunnable.setState(false);
            Log.d(DEBUGTAG, "release_test Flag.Activation= " + Flag.Activation.getState() + "line="
                    + Thread.currentThread().getStackTrace()[2].getLineNumber());
            Flag.Activation.setState(true);
            qtalkdb = new QtalkDB();
            ProvisionInfo provision_info = null;
            Cursor cs = qtalkdb.returnAllATEntry();
            cs.moveToFirst();
            int cscnt = 0;
            for (cscnt = 0; cscnt < cs.getCount(); cscnt++) {
                qtalk_settings.session = QtalkDB.getString(cs, "Session");
                qtalk_settings.key = QtalkDB.getString(cs, "Key");
                cs.moveToNext();
            }
            cs.close();
            Cursor cs1 = qtalkdb.returnAllPTEntry();
            cs1.moveToFirst();
            int cscnt1 = 0;
            for (cscnt1 = 0; cscnt1 < cs1.getCount(); cscnt1++) {
                qtalk_settings.lastProvisionFileTime = QtalkDB.getString(cs1, "lastProvisionFileTime");
                qtalk_settings.lastPhonebookFileTime = QtalkDB.getString(cs1, "lastPhonebookFileTime");
                cs1.moveToNext();
            }
            cs1.close();
            // qtalkdb.close();
            String path = Hack.getStoragePath() + "provision.xml";
            File file = new File(path);
            if (file.exists()) {
                try {
                    provision_info = ProvisionUtility.parseProvisionConfig(path);
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    Log.e(DEBUGTAG, "parseProvisionConfig", e);
                }
                if (provision_info != null)
                    qtalk_settings.provisionServer = provision_info.provision_uri;
                else {
                    Message msg = new Message();
                    msg.what = FAIL;
                    Bundle msgBundle = new Bundle();
                    msgBundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
                    msg.setData(msgBundle);
                    mProvisionHandler.sendMessage(msg);
                    return;
                }
            }
            Log.d(DEBUGTAG, "provisionServer 2:" + qtalk_settings.provisionServer);
        }
        try {
            qtalk_settings.session = session;
            updateSetting(qtalk_settings, false);
            new Thread(new Runnable() {

                @Override
                public void run() {
                    try {
                        qtnProvisionMessenger = new QThProvisionUtility(mProvisionHandler, null);
                        Time t = new Time();
                        t.setToNow();
                        provision_uri = qtalk_settings.provisionServer + qtalk_settings.provisionType
                                + "&service=provision&action=check&uid=" + qtalk_settings.provisionID + "&session="
                                + qtalk_settings.session + "&timestamp=" + qtalk_settings.lastProvisionFileTime;
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "provision url:" + provision_uri);
                        qtnProvisionMessenger.httpsConnect(provision_uri, session, qtalk_settings.lastProvisionFileTime,
                                qtalk_settings.provisionID, qtalk_settings.provisionType,
                                qtalk_settings.provisionServer, qtalk_settings.key);
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        Log.e(DEBUGTAG, "onStart", e);
                        Message msg = new Message();
                        msg.what = FAIL;
                        Bundle bundle = new Bundle();
                        bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
                        msg.setData(bundle);
                        mProvisionHandler.sendMessage(msg);
                    }
                }
            }).start();
        } catch (FailedOperateException e1) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "Provision context=null");
        }
        Log.d(DEBUGTAG, "<==onStart " + this.hashCode());
    }

    private void Activate(final String uid, final String psw, final String ipaddr, final String type,
                          final String deviceid,
                          final boolean controlKeepVisible, final boolean camDefault) {
        try {
            qtalk_settings = getSetting(false);
            String version = null;
            int versionCode = 0;
            try {
                version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
                versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
                versionNumber = version + "." + versionCode;
            } catch (NameNotFoundException e1) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "getVersion", e1);
                Log.e(DEBUGTAG, "", e1);
            }

            new Thread(new Runnable() {
                @Override
                public void run() {
                    // for test purpose "/pod/dispatch.do"
                    // "/~kakasi/provision/provision.php"
                    QThProvisionUtility qtnMessenger = new QThProvisionUtility(mActivateHandler, null);
                    QtalkDB qtalkdb = new QtalkDB();

                    ProvisionInfo provision_info = null;
                    Cursor cs = qtalkdb.returnAllATEntry();
                    // qtalkdb.insertATEntry(qtalk_settings.provisionServer,qtalk_settings.provisionID,qtalk_settings.versionNumber);
                    int size = cs.getCount();
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "2 DB size=" + size);

                    String ttspath = Hack.getTTSFile();
                    synchronized (mLogoutLock) {
                        File ttsfile = new File(ttspath);
                        if (!ttsfile.exists()) {
                            if (ProvisionSetupActivity.debugMode)
                                Log.d(DEBUGTAG, "ttsInfo is not exist send intent:" + LauncherReceiver.PTA_OP_GET);
                            Intent intent = new Intent();
                            intent.setAction(LauncherReceiver.PTA_OP_GET);
                            sendBroadcast(intent);
                        } else {
                            if (ProvisionSetupActivity.debugMode)
                                Log.d(DEBUGTAG, "ttsInfo is exist, read ttsInfo");
                            try {
                                BufferedReader br = new BufferedReader(new FileReader(ttspath));
                                String ptmsServer = br.readLine();
                                Log.d(DEBUGTAG, "ptmsServer=" + ptmsServer);
                                String tts_deviceId = br.readLine();
                                Log.d(DEBUGTAG, "tts_deviceId=" + tts_deviceId);
                                if (Hack.isUUID(tts_deviceId)) {
                                    QtalkSettings.ptmsServer = ptmsServer;
                                    QtalkSettings.tts_deviceId = tts_deviceId;
                                    if (ProvisionSetupActivity.debugMode)
                                        Log.d(DEBUGTAG, "ptmsServer=" + QtalkSettings.ptmsServer + " tts_deviceId="
                                                + QtalkSettings.tts_deviceId);

                                    if (Hack.setPTMSInfo(ptmsServer, tts_deviceId)) {
                                        synchronized (CommandService.this) {
                                            if (rsyslogRun != null) {
                                                rsyslogHandler.removeCallbacks(rsyslogRun);
                                                rsyslogRun = null;
                                            }
                                            rsyslogRun = new Runnable() {
                                                @Override
                                                public void run() {
                                                    Log.setSysLogConfigInLogback(QtalkSettings.ptmsServer,
                                                            QtalkSettings.tts_deviceId);
                                                }
                                            };
                                            rsyslogHandler.postDelayed(rsyslogRun, 6000);
                                        }
                                    }
                                } else {
                                    Log.e(DEBUGTAG, "1 device id format is not match id=" + tts_deviceId);
                                }
                            } catch (Exception e) {
                                Log.e(DEBUGTAG, "", e);
                            }
                        }
                    }
                    if (size > 0) {
                        Flag.SessionKeyExist.setState(true);
                        synchronized (mLogoutLock) {
                            String path = Hack.getStoragePath() + "provision.xml";
                            File file = new File(path);
                            if (file.exists()) {
                                try {
                                    provision_info = ProvisionUtility.parseProvisionConfig(path);
                                } catch (Exception e) {
                                    // TODO Auto-generated catch block
                                    Log.e(DEBUGTAG, "parseProvisionConfig", e);
                                }
                                ;
                                qtalk_settings.provisionID = uid;
                                qtalk_settings.provisionPWD = psw;
                                qtalk_settings.provisionServer = "https://" + ipaddr + "/thc/dis";
                                qtalk_settings.provisionType = type;
                                // mobile only has valid value
                                if (Hack.isUUID(deviceid))
                                    QtalkSettings.tts_deviceId = deviceid;
                                else
                                    Log.e(DEBUGTAG, "1 device id format is not match id=" + deviceid);

                                qtalk_settings.control_keep_visible = controlKeepVisible;
                                qtalk_settings.provisionCameraSwitch = camDefault;
                                qtalk_settings.provisionCameraSwitchReason = 1;
                                try {
                                    updateSetting(qtalk_settings, false);
                                } catch (FailedOperateException e) {
                                    // TODO Auto-generated catch block
                                    Log.e(DEBUGTAG, "Activate context=null 1");
                                }
                            }
                        }
                        cs.moveToFirst();
                        Log.d(DEBUGTAG, "release_test Flag.Activation= " + Flag.Activation.getState() + "line="
                                + Thread.currentThread().getStackTrace()[2].getLineNumber());
                        Flag.Activation.setState(true);
                        int cscnt = 0;
                        for (cscnt = 0; cscnt < cs.getCount(); cscnt++) {
                            qtalk_settings.session = QtalkDB.getString(cs, "Session");
                            qtalk_settings.key = QtalkDB.getString(cs, "Key");
                            // qtalk_settings.provisionServer =
                            // cs.getString(cs.getColumnIndex("Server"));
                            if (ProvisionSetupActivity.debugMode)
                                Log.d(DEBUGTAG, "Session:" + qtalk_settings.session);
                            if (ProvisionSetupActivity.debugMode)
                                Log.d(DEBUGTAG, "Key:" + qtalk_settings.key);
                            // Log.d(TAG,"Server:"+
                            // cs.getString(cs.getColumnIndex("Server")));
                            cs.moveToNext();
                        }
                        String login_url = qtalk_settings.provisionServer + qtalk_settings.provisionType
                                + "&service=activate&uid=" + qtalk_settings.provisionID + "&session="
                                + qtalk_settings.session;
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "login_url:" + login_url);
                        try {
                            qtnMessenger.httpsConnect(login_url, qtalk_settings.session, versionNumber);
                        } catch (IOException e) {
                            Log.d(DEBUGTAG, e.getMessage());
                            Message msg = new Message();
                            msg.what = LOGIN_FAILURE;
                            // qtalkdb.deactiveation();
                            // Log.d(TAG,"Delete all data!!!!!!!!!");
                            Bundle bundle = new Bundle();
                            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
                            msg.setData(bundle);
                            mActivateHandler.sendMessage(msg);
                        }
                    } else {
                        Flag.SessionKeyExist.setState(false);
                        qtalk_settings.provisionID = uid;
                        qtalk_settings.provisionPWD = psw;
                        qtalk_settings.provisionServer = "https://" + ipaddr + "/thc/dis";
                        qtalk_settings.provisionType = type;
                        // mobile only has valid value
                        if (Hack.isUUID(deviceid))
                            QtalkSettings.tts_deviceId = deviceid;
                        else
                            Log.e(DEBUGTAG, "1 device id format is not match id=" + deviceid);
                        qtalk_settings.provisionCameraSwitch = camDefault;
                        qtalk_settings.control_keep_visible = controlKeepVisible;
                        qtalk_settings.provisionCameraSwitchReason = 2;
                        try {
                            updateSetting(qtalk_settings, false);
                        } catch (FailedOperateException e) {
                            // TODO Auto-generated catch block
                            Log.e(DEBUGTAG, "Activate context=null 2");
                        }
                        Log.d(DEBUGTAG, "release_test Flag.Activation= " + Flag.Activation.getState() + "line="
                                + Thread.currentThread().getStackTrace()[2].getLineNumber());
                        Flag.Activation.setState(false);
                        synchronized (mLogoutLock) {
                            String Storage = Hack.getStoragePath();
                            File file = new File(Hack.getStoragePath());
                            if (file.exists()) {
                                FileUtility.delFolder(Hack.getStoragePath());
                                Intent intent = new Intent();
                                intent.setAction(LauncherReceiver.PTA_OP_GET);
                                sendBroadcast(intent);
                            }
                        }
                        String provision_url = qtalk_settings.provisionServer + qtalk_settings.provisionType
                                + "&service=activate&uid=" + qtalk_settings.provisionID + "&confirm="
                                + qtalk_settings.provisionPWD;
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "provision_url:" + provision_url);
                        try {
                            qtnMessenger.httpsConnect(provision_url, qtalk_settings.provisionServer,
                                    qtalk_settings.provisionID, qtalk_settings.provisionType, versionNumber);
                        } catch (IOException e) {
                            Log.d(DEBUGTAG, e.getMessage());
                            Message msg = new Message();
                            msg.what = LOGIN_FAILURE;
                            Bundle bundle = new Bundle();
                            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
                            msg.setData(bundle);
                            mActivateHandler.sendMessage(msg);
                        }

                    }
                    cs.close();
                    // qtalkdb.close();

                }
            }).start();

        } catch (FailedOperateException e) {
            Log.e(DEBUGTAG, "onResume", e);
            Message msg = new Message();
            msg.what = LOGIN_FAILURE;
            Bundle bundle = new Bundle();
            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
            msg.setData(bundle);
            mActivateHandler.sendMessage(msg);
        }
    }

    private void sendLoginCBBroadcast(int KeyReason) {
        Intent intent_cb = new Intent();
        intent_cb.setAction("android.intent.action.vc.login.cb");
        Bundle bundle = new Bundle();
        if (KeyReason == LOGIN_CB_OK)
            bundle.putString("KEY_STATE", "SUCCESS");
        else {
            bundle.putString("KEY_STATE", "FAIL");
            bundle.putInt("KEY_REASON", KeyReason);
        }
        intent_cb.putExtras(bundle);
        sendBroadcast(intent_cb);
    }
    // --------------login
    // function---------------------------------------------------------------

    // -------------logout
    // function---------------------------------------------------------------
    private void Logout(String login_after) {
        // Deactivate
        Log.d(DEBUGTAG, "==>Logout Its going to Deactivate on QtalkMain");
        HistoryDataBaseUtility historydatabase = null;
        QTService.phonebookhashcode = 0;
        if (isDeactivate == true) {
            Log.d(DEBUGTAG, "Now is during deactivation already. Do nothing.");
            return;
        }
        synchronized (mLogoutLock) {
            isDeactivate = true; // is not a good method
            QtalkSettings.nurseCall = false;
            sendLogoutRequest();
            CommandService.this.stopService(new Intent(CommandService.this, QTService.class));
            Log.d(DEBUGTAG, "stopService");
            Flag.Reprovision.setState(true);
            if (ProvisionSetupActivity.debugMode)
                Log.d(DEBUGTAG, " It can deactivate now!!");
            Activity LastActivity = ViroActivityManager.getLastActivity();
            if (LastActivity != null)
                LastActivity.finish();
            // CommandService.this.stopService(new
            // Intent(CommandService.this,QTService.class));

            QtalkDB qtalkdb = new QtalkDB();
            QtalkSettings qtalk_settings = null;
            try {
                qtalk_settings = getSetting(false);
            } catch (FailedOperateException e1) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e1);
            }
            FileUtility.delFolder(Hack.getStoragePath());
            qtalkdb.deactiveation();
            exitQT();
            Flag.SIPRunnable.setState(false);

            if (QThProvisionUtility.provision_info != null) {
                QThProvisionUtility.provision_info.lastProvisionFileTime = "";
                qtalk_settings.lastProvisionFileTime = "0";

                qtalk_settings.lastPhonebookFileTime = "0";
                QThProvisionUtility.provision_info.lastPhonebookFileTime = "";
                try {
                    updateSetting(QThProvisionUtility.provision_info);
                } catch (FailedOperateException e) {
                    // TODO Auto-generated catch block
                    Log.e(DEBUGTAG, "Logout context=null");
                }
                PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);
            }
            if (historydatabase == null) {
                historydatabase = new HistoryDataBaseUtility(getApplicationContext());
                historydatabase.open(true);
            }
            historydatabase.deactivate();
            Log.d(DEBUGTAG, "release_test Flag.Activation= " + Flag.Activation.getState() + "line="
                    + Thread.currentThread().getStackTrace()[2].getLineNumber());
            Flag.Activation.setState(false);
            Flag.SessionKeyExist.setState(false);
            PreferenceManager.getDefaultSharedPreferences(this).edit().clear().commit(); // clear
            // preference
            this.getSharedPreferences("EXPAND", 0).edit().clear().commit();
            // ---------------------------------------------------------------
            if (login_after != null && login_after.contentEquals("YES")) {
                Intent broadcast_intent = new Intent();
                broadcast_intent.setAction("android.intent.action.vc.logout.cb.login.after");

                Log.d(DEBUGTAG, "Send Broadcast to mylauncher logout SUCCESS");
                sendBroadcast(broadcast_intent);

            } else {
                doMobile(QThProvisionUtility.MOBILE_ACTION_CLEAR);
                Intent broadcast_intent = new Intent();
                broadcast_intent.setAction("android.intent.action.vc.logout.cb");
                Bundle bundle = new Bundle();
                bundle.putString("KEY_LOGOUT_CB", "SUCCESS");
                broadcast_intent.putExtras(bundle);
                Log.d(DEBUGTAG, "Send Broadcast to vc logout SUCCESS");
                setVCLOGIN(false);
                sendBroadcast(broadcast_intent);
            }
            // ---------------------------------------------------------------
            isDeactivate = false;
            Log.d(DEBUGTAG, "<==Logout Its going to Deactivate on QtalkMain");
        }
    }

    private void sendLogoutRequest() {
        try {
            final QtalkSettings settings = QTReceiver.engine(CommandService.this, false)
                    .getSettings(CommandService.this);
            provisionID = settings.provisionID;
            userID = settings.userID;
            provisionType = settings.provisionType;

        } catch (FailedOperateException e1) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "", e1);
        }
        if (provisionID == null) {
            Log.e(DEBUGTAG, "uid=null skip do logout req.");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                QThProvisionUtility qtnMessenger = new QThProvisionUtility(mLogoutHandler, null);
                QtalkDB qtalkdb = new QtalkDB();

                if (QThProvisionUtility.provision_info != null) {
                    logout_uri = QThProvisionUtility.provision_info.phonebook_uri + provisionType + "&service=sip&uid="
                            + provisionID + "&action=unregister" + "&sip_id=" + userID;
                    try {
                        qtnMessenger.httpsConnect(logout_uri);
                    } catch (Exception e) {
                        Log.e(DEBUGTAG, "Send logout HTTPS to server", e);
                    }
                } else {
                    String provision_file = Hack.getStoragePath() + "provision.xml";
                    try {
                        QThProvisionUtility.provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
                    } catch (Exception e) {
                        // TODO Auto-generated catch block
                        Log.e(DEBUGTAG, "", e);
                    }
                }
            }
        }).start();

    }

    protected QtalkSettings getSetting(final boolean enableService) throws FailedOperateException {
        // Read settings
        return QTReceiver.engine(CommandService.this, enableService).getSettings(CommandService.this);
    }

    protected void exitQT() {
        QTReceiver.finishQT(CommandService.this);
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "playSoundLogout");
        RingingUtil.playSoundLogout(CommandService.this);
    }

    protected void updateSetting(final ProvisionInfo provisionInfo) throws FailedOperateException {
        QtalkEngine engine = QTReceiver.engine(CommandService.this, false);
        if (engine != null) {
            engine.updateSetting(CommandService.this, provisionInfo);
        }
    }

    protected void updateSetting(final QtalkSettings settings, final boolean enableService)
            throws FailedOperateException {
        QtalkEngine engine = QTReceiver.engine(CommandService.this, enableService);
        if (engine != null) {
            try {
                // Write settings & re-login
                engine.updateSetting(CommandService.this, settings);
            } catch (FailedOperateException e) {
                Log.e(DEBUGTAG, "updateSetting", e);
                Toast.makeText(CommandService.this, e.toString(), Toast.LENGTH_LONG);
            }
        }
    }
    // -------------logout
    // function---------------------------------------------------------------

}
