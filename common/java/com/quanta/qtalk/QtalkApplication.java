package com.quanta.qtalk;

import com.quanta.qtalk.media.MediaEngine;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

public class QtalkApplication extends Application {
    //    private static final String TAG = "QtalkApplication";
    private static final String DEBUGTAG = "QTFa_QtalkApplication";
    private final static Object mSyncObject = new Object();
    private final static java.util.Map<String, MediaEngine> mMediaEngineTable = new java.util.TreeMap<>();

    public MediaEngine getMediaEngine(String callID) {
        MediaEngine result = null;
        synchronized (mSyncObject) {
            if (callID != null) {
                result = mMediaEngineTable.get(callID);
                if (result == null) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "new MediaEngine:" + callID + "@" + this.hashCode() + " size:" + mMediaEngineTable.size());
                    result = new MediaEngine();
                    mMediaEngineTable.put(callID, result);
                }
            }
        }
        return result;
    }

    public void destoryMediaEngine(String callID) {
        synchronized (mSyncObject) {
            mMediaEngineTable.remove(callID);
        }
    }

    private static Context appContext;

    @Override
    public void onCreate() {
        super.onCreate();
        appContext = getApplicationContext();

        SharedPreferences prefs = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        boolean isFirstLaunch = prefs.getBoolean("isFirstLaunch", true);

        if (isFirstLaunch) {
            // 發送更新完成的 Intent
            Intent updateCompletedIntent = new Intent("com.quanta.qtalk.UPDATE_COMPLETED");
            sendBroadcast(updateCompletedIntent);

            // 標記應用程式已經啟動過
            prefs.edit().putBoolean("isFirstLaunch", false).apply();
        }
        Hack.init(this);
    }

    public static Context getAppContext() {
        return appContext;
    }
}
