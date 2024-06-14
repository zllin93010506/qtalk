package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.text.TextUtils;

import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.QtalkMain;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.LogCompress;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.TreeMap;

public class LauncherReceiver extends BroadcastReceiver {

    private Handler mHandler = new Handler();
    private static Bundle mBundle = null;
    private static boolean isDeactivate = false;
    private static boolean isLogin = false;
    // if isForcedLogin = true then skip notification vc login 
    private static boolean isForcedLogin = false;
    private static String mLoginId = "";
    private static final String DEBUGTAG = "QSE_LauncherReceiver";

    private static final String KEY_COMMAND = "KEY_COMMAND";
    private static final String KEY_LOGIN_AFTER = "KEY_LOGIN_AFTER";
    private static final String KEY_OPEN_HISTORY = "KEY_OPEN_HISTORY";

    private static final String KEY_LOGIN_UID = "KEY_LOGIN_UID";
    private static final String KEY_LOGIN_PSWD = "KEY_LOGIN_PSWD";
    private static final String KEY_LOGIN_IPADDR = "KEY_LOGIN_IPADDR";
    private static final String KEY_NURSE_UID = "KEY_NURSE_UID";
    private static final String KEY_NURSECALL = "KEY_NURSECALL";
    private static final String KEY_MOBILE = "KEY_MOBILE";
    private static final String KEY_TTS_DEVICE_ID = "KEY_TTS_DEVICE_ID";
    private static final String KEY_RESOURCE_CHANGED = "KEY_RESOURCE_CHANGED";
    private static Calendar mNewCallCalendar = null;

    private static Handler rsyslogHandler = new Handler();
    private static Runnable rsyslogRun = null;
    private static final Object rsyslogLock = new Object();

    private static final Object makecallLock = new Object();

    public static final String ACTION_VC_LOGIN = "android.intent.action.vc.login";
    public final static String ACTION_VC_MESSAGE = "android.intent.action.vc.message";
    public static final String ACTION_VC_GET_LOGIN_DATA = "android.intent.action.vc.get.login.data";
    public static final String ACTION_VC_RETURN_LOGIN_DATA = "android.intent.action.vc.return.login.data";
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
    public final static String PTA_RESOURCE_CHANGE = "pta.resource.change";

    private void hangupCall(Context context, Intent intent) {
        Log.d(DEBUGTAG, "LauncherReceiver.hangupCall()");
        Intent myIntent = new Intent(context, CommandService.class);
        Bundle bundle = new Bundle();
        bundle.putInt(KEY_COMMAND, CommandService.command_handsethangup);
        myIntent.putExtras(bundle);
        context.startService(myIntent);

        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            Log.e(DEBUGTAG, "", e);
        }
    }

    private void pickupCall(Context context, Intent intent) {
        Log.d(DEBUGTAG, "LauncherReceiver.pickupCall()");
        Intent myIntent = new Intent(context, CommandService.class);
        Bundle bundle = new Bundle();
        bundle.putInt(KEY_COMMAND, CommandService.command_handsetpickup);
        myIntent.putExtras(bundle);
        context.startService(myIntent);
    }

    private void handleReceive(Context context, Intent intent) {
		/*
		Intent testIntent = new Intent(context, CommandService.class);
		Bundle bundle_test = new Bundle();
		bundle_test.putInt("KEY_COMMAND", CommandService.command_test);
		bundle_test.putString("source", DEBUGTAG);
		testIntent.putExtras(bundle_test);
		context.startService(testIntent);
		Log.d(DEBUGTAG,"send testcommand");
		*/
        final String action = intent.getAction();

        Log.d(DEBUGTAG, "action:" + action);

        if (TextUtils.equals(action, Intent.ACTION_BOOT_COMPLETED) ||
                TextUtils.equals(action, Intent.ACTION_USER_PRESENT) ||
                TextUtils.equals(action, QtalkAction.ACTION_APP_START)) {
            Intent myIntent = new Intent(context, LauncherReceiver.class);
            context.startService(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_MESSAGE)) {
            Intent myIntent = new Intent(context, CommandService.class);
            mBundle = intent.getExtras();
            myIntent.putExtras(mBundle);
            context.startService(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGIN) || TextUtils.equals(action, LauncherReceiver.ACTION_VC_RETURN_LOGIN_DATA)) {
            Log.d(DEBUGTAG, "isDeactive: " + isDeactivate + ", isLogin:" + isLogin + ", Action: " + action);
            mBundle = intent.getExtras();
            String uid = mBundle.getString("KEY_LOGIN_UID", "");
            String type = mBundle.getString("KEY_LOGIN_TYPE", "");
            if(TextUtils.isEmpty(QtalkSettings.type) || !TextUtils.equals(QtalkSettings.type, type)) {
                QtalkSettings.type = type;
			}
            Log.d(DEBUGTAG, "KEY_LOGIN_UID=" + uid + ", mLoginId=" + mLoginId);
            if (!isLogin || (TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGIN) && !TextUtils.equals(mLoginId, uid))) {
                mHandler.postDelayed(login_status, 10000);

                if (isForcedLogin && Hack.isPhone() && TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGIN) && (!mBundle.getBoolean("KEY_START_LOGIN", true))) {
                    //Log.d(DEBUGTAG,"<warning> skip Login command, because it is notification login");
                    //return;
                    PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
                    PowerManager.WakeLock wakeLock = powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK
                            | PowerManager.ACQUIRE_CAUSES_WAKEUP, "qtalk:LauncherReceiver");
                    if (!wakeLock.isHeld())
                        wakeLock.acquire();
                    if (wakeLock.isHeld())
                        wakeLock.release();
                }
                isDeactivate = true;
                if (mBundle.containsKey("KEY_LOGIN_UID")) {
                    Log.d(DEBUGTAG, "KEY_LOGIN_UID=" + mBundle.getString("KEY_LOGIN_UID"));

                    if ((uid == null) || (uid.length() < 2))
                        return;
                } else {
                    Log.d(DEBUGTAG, "no KEY_LOGIN_UID");
                    Intent broadcast_intent = new Intent();
                    broadcast_intent.setAction("android.intent.action.vc.logout.cb.login.after");

                    Log.d(DEBUGTAG, "Send Broadcast to mylauncher logout SUCCESS");
                    context.sendBroadcast(broadcast_intent);
                    return;
                }
                mLoginId = uid;
                isLogin = true;
                Intent myIntent = new Intent(context, CommandService.class);
                Bundle logout_bundle = new Bundle();
                logout_bundle.putInt(KEY_COMMAND, CommandService.command_logout);
                logout_bundle.putString(KEY_LOGIN_AFTER, "YES");
                if (Hack.isPhone())
                    logout_bundle.putBoolean(KEY_MOBILE, true);
                logout_bundle.putBoolean("KEY_START_ACTIVITY", mBundle.getBoolean("KEY_START_ACTIVITY"));
                myIntent.putExtras(logout_bundle);
                context.startService(myIntent);
                Hack.startNursingApp(context);
                isForcedLogin = true;
            } else {
                Log.d(DEBUGTAG, "<warning> skip Login command, because now is in Login progress");
                return;
            }
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_MAKE_CALL)) {
            synchronized (makecallLock) {
                if (mNewCallCalendar == null) {
                    mNewCallCalendar = Calendar.getInstance();
                    mNewCallCalendar.add(Calendar.SECOND, 3);
                } else {
                    Calendar now = Calendar.getInstance();
                    if (mNewCallCalendar.compareTo(now) < 0) {
                        mNewCallCalendar.clear();
                        now.add(Calendar.SECOND, 3);
                        mNewCallCalendar = now;
                    } else {
                        SimpleDateFormat sdFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS");
                        Log.d(DEBUGTAG, "the second make/nurse call is not over 3 seconds. now=" + sdFormat.format(now.getTime()) + " lastest call=" + sdFormat.format(mNewCallCalendar.getTime()));
                        return;
                    }
                }
                hangupCall(context, intent);
                Bundle bundle = new Bundle();
                bundle = intent.getExtras();
                Intent MakeCallIntent = new Intent(context, CommandService.class);
                bundle.putInt(KEY_COMMAND, CommandService.command_makecall);
                MakeCallIntent.putExtras(bundle);
                context.startService(MakeCallIntent);
            }
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_NURSE_CALL)) {
            synchronized (makecallLock) {
                if (mNewCallCalendar == null) {
                    mNewCallCalendar = Calendar.getInstance();
                    mNewCallCalendar.add(Calendar.SECOND, 3);
                } else {
                    Calendar now = Calendar.getInstance();
                    if (mNewCallCalendar.compareTo(now) < 0) {
                        mNewCallCalendar.clear();
                        now.add(Calendar.SECOND, 3);
                        mNewCallCalendar = now;
                    } else {
                        SimpleDateFormat sdFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS");
                        Log.d(DEBUGTAG, "the second make/nurse call is not over 3 seconds. now=" + sdFormat.format(now.getTime()) + " lastest call=" + sdFormat.format(mNewCallCalendar.getTime()));
                        return;
                    }
                }
                hangupCall(context, intent);

                Bundle bundle = new Bundle();
                bundle = intent.getExtras();
                Log.d(DEBUGTAG, "set mBundle");
                if (bundle != null) {
                    if (bundle.getString(KEY_NURSE_UID) != null) {
                        bundle.putString(KEY_NURSECALL, "YES");
                        bundle.putInt(KEY_COMMAND, CommandService.command_nursecall);
                        Intent myIntent = new Intent(context, CommandService.class);
                        myIntent.putExtras(bundle);
                        context.startService(myIntent);
                    } else {
                        Log.d(DEBUGTAG, "<error> KEY_NURSE_UID is NULL");
                    }
                } else {
                    Log.d(DEBUGTAG, "<error> ACTION_VC_NURSE_CALL is NULL");
                }
            }
        } else if (TextUtils.equals(action, "android.intent.action.vc.login.cb")) {
            isLogin = false;
            mHandler.removeCallbacks(login_status);
        } else if (TextUtils.equals(action, "android.intent.action.vc.logout.cb.login.after")) {
            if (mBundle != null) {
                Intent myIntent = new Intent(context, CommandService.class);
                mBundle.putInt(KEY_COMMAND, CommandService.command_login);
                if (Hack.isPhone())
                    mBundle.putBoolean(KEY_MOBILE, TextUtils.equals(action, "android.intent.action.vc.logout.cb.login.after"));
                myIntent.putExtras(mBundle);
                context.startService(myIntent);
                isDeactivate = false;
            } else {
                Log.e(DEBUGTAG, "<error> no Bundle for login when screen on");
            }
            mBundle = null;
        } else if (TextUtils.equals(action, Intent.ACTION_SCREEN_OFF)) {
            Log.d(DEBUGTAG, "do ACTION_SCREEN_OFF");
            if (Hack.isPhone()) {
                Log.d(DEBUGTAG, "send command_sleep");
                Intent myIntent = new Intent(context, CommandService.class);
                Bundle sleep_bundle = new Bundle();
                sleep_bundle.putInt(KEY_COMMAND, CommandService.command_sleep);
                myIntent.putExtras(sleep_bundle);
                context.startService(myIntent);
            }
        } else if (TextUtils.equals(action, Intent.ACTION_SCREEN_ON)) {
            Log.d(DEBUGTAG, "do ACTION_SCREEN_ON");
            if (Hack.isPhone()) {
                Log.d(DEBUGTAG, "send command_wake");
                Intent myIntent = new Intent(context, CommandService.class);
                Bundle wake_bundle = new Bundle();
                wake_bundle.putInt(KEY_COMMAND, CommandService.command_wake);
                myIntent.putExtras(wake_bundle);
                context.startService(myIntent);
            }
        } else if (TextUtils.equals(action, "android.intent.action.vc.logout.cb")) {

        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGOUT)) {
            if (Hack.isPhone() && TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGOUT)) {
                mBundle = null;
                Log.d(DEBUGTAG, "clear mBundle 2");
            }
            Intent myIntent = new Intent(context, CommandService.class);
            Bundle logout_bundle = new Bundle();
            logout_bundle.putInt(KEY_COMMAND, CommandService.command_logout);
            logout_bundle.putString(KEY_LOGIN_AFTER, "NO");
            if (Hack.isPhone())
                logout_bundle.putBoolean(KEY_MOBILE, TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGOUT));
            myIntent.putExtras(logout_bundle);
            context.startService(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_TTS_LIST)) {
            Bundle bundle = intent.getExtras();
			/*if(bundle!=null){
				ArrayList<String> ttsList = bundle.getStringArrayList("KEY_TTS_MSG");
				if(ttsList!=null){
					writeToLocalList(ttsList);
					
				}else{
					Log.d(DEBUGTAG,"<error> KEY_TTS_MSG is NULL");
				}
			}else{
				Log.d(DEBUGTAG,"<error> ACTION_VC_TTS_LIST is NULL");
			}*/
            updateTTS();
            String tts_device_id = bundle.getString("KEY_TTS_DEVICE_ID");
            String tts_ptms_ip = bundle.getString("KEY_TTS_PTMS_IP");
            Log.d(DEBUGTAG, "ACTION_VC_TTS_LIST tts_device_id=" + tts_device_id + ", tts_ptms_ip" + tts_ptms_ip);
            if (tts_device_id != null && tts_ptms_ip != null) {
                if (Hack.setPTMSInfo(tts_ptms_ip, tts_device_id)) {
                    writeToLocal(tts_ptms_ip, tts_device_id);
                    synchronized (rsyslogLock) {
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
                    sendRsyslogIntent(context);
                }
            }
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_OPEN_CONTACT)) {
            Intent myIntent = new Intent(context, QtalkMain.class);
            Bundle bundle = new Bundle();
            bundle.putString(KEY_OPEN_HISTORY, "NO");
            myIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            myIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
            myIntent.putExtras(bundle);
            context.startActivity(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_OPEN_HISTORY)) {
            Intent myIntent = new Intent(context, QtalkMain.class);
            Bundle bundle = new Bundle();
            bundle.putString(KEY_OPEN_HISTORY, "YES");
            myIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            myIntent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
            myIntent.putExtras(bundle);
            context.startActivity(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_LOGON_STATE)) {
            Bundle bundle = new Bundle();
            bundle.putInt(KEY_COMMAND, CommandService.command_logonstate);
            Intent myIntent = new Intent(context, CommandService.class);
            myIntent.putExtras(bundle);
            context.startService(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.HISTORY_TTS_FROM_LAUNCHER)) {
            Bundle bundle = new Bundle();
            bundle = intent.getExtras();
            bundle.putInt(KEY_COMMAND, CommandService.command_ttslog);
            Intent myIntent = new Intent(context, CommandService.class);
            myIntent.putExtras(bundle);
            context.startService(myIntent);

        } else if (TextUtils.equals(action, LauncherReceiver.HANDSET_PICKUP)) {
            pickupCall(context, intent);

        } else if (TextUtils.equals(action, LauncherReceiver.HANDSET_HANGUP)) {
            hangupCall(context, intent);
        } else if (TextUtils.equals(action, LauncherReceiver.PTA_OP_GET_RC)) {
            Bundle bundle = intent.getExtras();
            String tts_device_id = bundle.getString("PT_ID");
            String tts_ptms_ip = bundle.getString("PT_PTMS_IP");
            Log.d(DEBUGTAG, "PTA_OP_GET_RC tts_device_id=" + tts_device_id + ", tts_ptms_ip" + tts_ptms_ip);
            if (tts_device_id != null && tts_ptms_ip != null) {
                if (Hack.setPTMSInfo(tts_ptms_ip, tts_device_id)) {
                    writeToLocal(tts_ptms_ip, tts_device_id);
                    sendRsyslogIntent(context);
                }
            }
        } else if (TextUtils.equals(action, LauncherReceiver.PTA_RESOURCE_CHANGE)) {
            String changedResource = intent.getStringExtra("RESOURCE_CHANGE");
            Log.d(DEBUGTAG, "ServiceChanged RCS Resource changed " + changedResource);
            Intent myIntent = new Intent(context, CommandService.class);
            Bundle bundle = new Bundle();
            bundle.putInt(KEY_COMMAND, CommandService.command_resourcechanged);
            bundle.putString(KEY_RESOURCE_CHANGED, changedResource);
            myIntent.putExtras(bundle);
            context.startService(myIntent);
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_GET_TTS)) {
            Log.d(DEBUGTAG, "ACTION_GET_TTS received");
            updateTTS();
        } else if (TextUtils.equals(action, LauncherReceiver.ACTION_VC_ALARM)) {
            Log.d(DEBUGTAG, "ACTION_VC_ALARM received");
            Hack.set_alarm(context, LauncherReceiver.class, LauncherReceiver.ACTION_VC_ALARM, 60 * 60 * 8);
            new Thread(new Runnable() {
                @Override
                public void run() {
                    LogCompress.handleLogs();
                }
            }).start();
			/*
			Intent alarmIntent = new Intent(context, CommandService.class);
			Bundle alarmBundle = new Bundle();
			alarmBundle.putInt(KEY_COMMAND, CommandService.command_alarm);
			alarmIntent.putExtras(alarmBundle);
			context.startService(alarmIntent);
			 */
        }

    }

    @SuppressLint("ShowToast")
    @Override
    public void onReceive(final Context context, final Intent intent) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                handleReceive(context, intent);
            }
        }).start();
    }

    private void writeToLocalList(ArrayList<String> ttsList) {
        String path = Hack.getStoragePath() + "ttsList";
        File dirFile = new File(Hack.getStoragePath());
        Log.d(DEBUGTAG, "ttsList exists: " + dirFile.exists());
        if (!dirFile.exists()) {
            dirFile.mkdir();
        }
        File listFile = new File(path);
        FileWriter FW = null;
        try {
            FW = new FileWriter(listFile);
            for (int i = 0; i < ttsList.size(); i++) {
                //Log.d(DEBUGTAG,"ttsList: "+i+" "+ttsList.get(i).toString());
                FW.write(ttsList.get(i).toString());
                FW.write('\n');

            }
            FW.close();
        } catch (IOException e) {
            Log.e(DEBUGTAG, "Writing ttslist error!!", e);
        }
    }

    private void writeToLocal(String ptmsIp, String deviceId) {
        String path = Hack.getStoragePath() + "ttsInfo";
        File dirFile = new File(Hack.getStoragePath());
        Log.d(DEBUGTAG, "ttsInfo exists: " + dirFile.exists());
        if (!dirFile.exists()) {
            dirFile.mkdir();
        }
        File listFile = new File(path);
        FileWriter FW = null;
        try {
            FW = new FileWriter(listFile);
            FW.write(ptmsIp);
            FW.write('\n');
            FW.write(deviceId);
            FW.write('\n');
            FW.flush();
            FW.close();
        } catch (IOException e) {
            Log.e(DEBUGTAG, "Writing ptmsIp deviceId error!!", e);
        }
    }

    private Runnable login_status = new Runnable() {

        @Override
        public void run() {
            Log.d(DEBUGTAG, "Time out, login available");
            isLogin = false;
        }
    };


    private ArrayList<HashMap<String, String>> SortByPriority(ArrayList<HashMap<String, String>> items,
                                                              boolean timeSort) {
        int i, j;
        HashMap<String, String> temp;
        for (i = 0; i < items.size() - 1; i++) {
            for (j = i + 1; j < items.size(); j++) {
                if (Integer.valueOf(items.get(i).get("priority")) > Integer.valueOf(items.get(j).get("priority"))) {
                    temp = items.get(i);
                    items.set(i, items.get(j));
                    items.set(j, temp);
                }
            }
        }

        if (items.size() == 1)
            return items;

        if (timeSort) {
            String p = "";
            ArrayList<HashMap<String, String>> byPriority = new ArrayList<HashMap<String, String>>();
            ArrayList<HashMap<String, String>> result = new ArrayList<HashMap<String, String>>();
            for (int m = 0; m < items.size(); m++) {
                if (!p.equals(items.get(m).get("priority"))) {
                    p = items.get(m).get("priority");
                    if (byPriority.size() > 0) {
                        for (HashMap<String, String> byPrioityItem : SortByTimestamp(byPriority)) {
                            result.add(byPrioityItem);
                        }
                    }
                    byPriority = null;
                    byPriority = new ArrayList<HashMap<String, String>>();
                    byPriority.add(items.get(m));
                } else if (p.equals(items.get(m).get("priority"))) {
                    byPriority.add(items.get(m));
                }

                if (m == items.size() - 1) {
                    for (HashMap<String, String> byPrioityItem : SortByTimestamp(byPriority)) {
                        result.add(byPrioityItem);
                    }
                }

            }
            // Log.d(TAG, "[final result by time]: "+result.toString());
            return result;
        } else {
            return items;
        }

    }

    private ArrayList<HashMap<String, String>> SortByTimestamp(ArrayList<HashMap<String, String>> items) {
        int i, j;
        HashMap<String, String> temp;
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm'Z'");
        for (i = 0; i < items.size() - 1; i++) {
            for (j = i + 1; j < items.size(); j++) {

                try {
                    Date d1 = df.parse(items.get(i).get("created"));
                    Date d2 = df.parse(items.get(j).get("created"));
                    if (d1.compareTo(d2) <= 0) {
                        temp = items.get(i);
                        items.set(i, items.get(j));
                        items.set(j, temp);
                    }
                } catch (ParseException e) {
                    Log.e(DEBUGTAG, "", e);
                }

            }
        }
        return items;
    }

    public ArrayList<String> configProcessPhoneNotification(JSONArray notification) {
        HashMap<String, ArrayList<HashMap<String, String>>> notifyAll = new HashMap<String, ArrayList<HashMap<String, String>>>();

        try {
            for (int i = 0; !notification.isNull(i); i++) {
                JSONObject obj;
                String category;

                obj = notification.getJSONObject(i);
                category = obj.getString("category");

                HashMap<String, String> item = new HashMap<String, String>();

                if (notifyAll.containsKey(category) == false) {
                    notifyAll.put(category, new ArrayList<HashMap<String, String>>());
                }

                item.put("id", obj.getString("id"));
                item.put("title", obj.getString("title"));
                item.put("priority", obj.getString("priority"));
                Log.d(DEBUGTAG, "id:" + obj.getString("id"));
                Log.d(DEBUGTAG, "title:" + obj.getString("title"));
                Log.d(DEBUGTAG, "priority:" + obj.getString("priority"));
                notifyAll.get(category).add(item);
            }
        } catch (JSONException e) {
            Log.d(DEBUGTAG, "JSONException : " + e.toString());
        }

        TreeMap<String, ArrayList<HashMap<String, String>>> sorted = new TreeMap<String, ArrayList<HashMap<String, String>>>(
                notifyAll);
        Log.d(DEBUGTAG, "Wifi phone tts notification: " + notification.length());

        ArrayList<String> tCategoryPriority = new ArrayList<String>(sorted.keySet());
        Collections.sort(tCategoryPriority);
        ArrayList<String> ttsContent = new ArrayList<String>();

        if (sorted != null) {
            for (String category : tCategoryPriority) {
                // Log.d(TAG,"[MSG] TTS category: "+category);
                // Log.d(TAG,"[MSG] TTS content list:
                // "+sorted.get(category).toString());
                if (sorted.get(category) != null && sorted.get(category).size() > 0) {
                    for (int i = 0; i < SortByPriority(sorted.get(category), false).size(); i++) {
                        ttsContent.add(sorted.get(category).get(i).get("title").trim());
                        // Log.d(TAG,"[MSG] TTS content:
                        // "+sorted.get(category).get(i).get("title").trim());
                    }
                }
            }
        }


        return ttsContent;
    }


    synchronized public void getTTS(String ptmsIp, String deviceId) throws IOException {
        try {
            Log.d(DEBUGTAG, "httpsConnect(ptms tts), ptmsIp:" + ptmsIp + "deviceId" + deviceId);
            String url = "https://" + ptmsIp + "/api/v2/texttemplates?config__device=" + deviceId + "&category=TEXTTEMPLATE_NOTIFICATION&page_size=100";
            JSONArray articles = null;
            ArrayList<String> ttlStrings = new ArrayList<String>();
            while (url != null) {
                Log.d(DEBUGTAG, "httpsConnect(ptms tts), url:" + url);
                String result = Hack.httpsConnect(url);
                Log.d(DEBUGTAG, "httpsConnect(ptms tts), result:" + result);
                JSONObject json = new JSONObject(result);
                if (json.isNull("results")) {
                    Log.e(DEBUGTAG, "httpsConnect(ptms tts), no results in json");
                } else {
                    /*
                     * Iterator<String> keys = json.keys(); for
                     * (Iterator<String> i = keys; i.hasNext();) { String key =
                     * i.next(); Log.d(DEBUGTAG, "httpsConnect(ptms tts), key:"
                     * + key + " value:" + json.getString(key)); }
                     */
                    articles = json.getJSONArray("results");

                    ttlStrings.addAll(configProcessPhoneNotification(articles));
                }
                if (json.isNull("next")) {
                    url = null;
                } else {
                    url = json.getString("next");
                }
            }
            Log.d(DEBUGTAG, "getTTS(ptms tts), ttlStrings.size:" + ttlStrings.size());
            writeToLocalList(ttlStrings);
            Log.d(DEBUGTAG, "getTTS(ptms tts) exit loop");
        } catch (Exception ex) {
            Log.e(DEBUGTAG, "", ex);
        }
    }

    private String ptmsIp = null;
    private String deviceId = null;
    private Runnable getTTSThread = new Runnable() {
        public void run() {
            try {
                getTTS(ptmsIp, deviceId);
            } catch (IOException e) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e);
            }
        }
    };

    private void sendRsyslogIntent(Context context) {
        synchronized (rsyslogLock) {
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
        Intent rbIntent = new Intent(context, CommandService.class);
        Bundle rbundle = new Bundle();
        rbundle.putInt(KEY_COMMAND, CommandService.command_rsyslogconfig);
        rbundle.putString("KEY_DEVICEID", QtalkSettings.tts_deviceId);
        rbundle.putString("KEY_PTMSSERVER", QtalkSettings.ptmsServer);
        rbIntent.putExtras(rbundle);
        context.startService(rbIntent);
    }

    protected void updateTTS() {
        String path = Hack.getTTSFile();
        try {
            BufferedReader br = new BufferedReader(new FileReader(path));
            ptmsIp = br.readLine();
            deviceId = br.readLine();
            Hack.setPTMSInfo(ptmsIp, deviceId);
            br.close();
            Thread thread = new Thread(getTTSThread);
            thread.start();
        } catch (Exception e) {
            Log.e(DEBUGTAG, "", e);
        }
    }
}