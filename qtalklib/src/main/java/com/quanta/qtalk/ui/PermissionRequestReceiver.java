package com.quanta.qtalk.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;

import com.quanta.qtalk.util.Log;

public class PermissionRequestReceiver extends BroadcastReceiver {
    public static String START_PERMISSION_REQUEST_ACTIVITY = "com.quanta.qtalk.ui.permission_request";
    private static final String DEBUGTAG = "QTFa_PermissionRequestReceiver";
    @Override
    public void onReceive(Context context, Intent intent) {
        // Check if the broadcast action matches the one sent from MainActivity
        if (intent.getAction() != null && TextUtils.equals(intent.getAction(), START_PERMISSION_REQUEST_ACTIVITY)) {
            Log.d(DEBUGTAG, START_PERMISSION_REQUEST_ACTIVITY);
            // Start MainActivity
            Intent startIntent = new Intent(context, PermissionRequestActivity.class);
            startIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(startIntent);
        }
    }
}
