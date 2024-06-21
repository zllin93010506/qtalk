package com.quanta.qtalk.ui;

import android.Manifest;
import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.R;

import java.util.ArrayList;
import java.util.List;

public class PermissionRequestActivity extends Activity {
    private static final int REQUEST_CODE_PERMISSIONS = 100;
    private static final String DEBUGTAG = "QTFa_PermissionRequestActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(DEBUGTAG,"onCreate");
        setContentView(R.layout.layout_permission_request);

        checkOverlayPermission();
        checkNotificationPermission();
        checkAndRequestPermissions();
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkOverlayPermission();
        checkAndRequestPermissions();
        Log.d(DEBUGTAG,"onResume");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(DEBUGTAG,"onPause");
    }

    public static void requestPermission(Context context) {
        Intent broadcastIntent = new Intent(context, PermissionRequestReceiver.class);
        broadcastIntent.setAction(PermissionRequestReceiver.START_PERMISSION_REQUEST_ACTIVITY);
        context.sendBroadcast(broadcastIntent);
    }

    private void checkOverlayPermission() {
        if (!Settings.canDrawOverlays(this)) {
            this.startActivityForResult(
                    new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                            Uri.parse("package:" + this.getPackageName())), Settings.ACTION_MANAGE_OVERLAY_PERMISSION.hashCode());
        }
    }
    static Intent getApplicationDetailsIntent(@NonNull Context context) {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        intent.setData(Uri.parse("package:" + context.getPackageName()));
        if (areActivityIntent(context, intent)) {
            return intent;
        }

        intent = new Intent(Settings.ACTION_APPLICATION_SETTINGS);
        if (areActivityIntent(context, intent)) {
            return intent;
        }

        intent = new Intent(Settings.ACTION_MANAGE_APPLICATIONS_SETTINGS);
        if (areActivityIntent(context, intent)) {
            return intent;
        }
        return new Intent(Settings.ACTION_SETTINGS);
    }
    static boolean areActivityIntent(@NonNull Context context, @Nullable Intent intent) {
        if (intent == null) {
            return false;
        }
        // 这里为什么不用 Intent.resolveActivity(intent) != null 来判断呢？
        // 这是因为在 OPPO R7 Plus （Android 5.0）会出现误判，明明没有这个 Activity，却返回了 ComponentName 对象
        PackageManager packageManager = context.getPackageManager();

        return !packageManager.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY).isEmpty();
    }
    static Intent getNotificationPermissionIntent(@NonNull Context context) {
        Intent intent = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS);

            intent.putExtra(Settings.EXTRA_APP_PACKAGE, context.getPackageName());
            //intent.putExtra(Settings.EXTRA_CHANNEL_ID, context.getApplicationInfo().uid);

            if (!areActivityIntent(context, intent)) {
                intent = getApplicationDetailsIntent(context);
            }
        }
        return intent;
    }
    private void checkNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            if (!getSystemService(NotificationManager.class).areNotificationsEnabled()) {
                this.startActivityForResult(getNotificationPermissionIntent(this), Settings.ACTION_APP_NOTIFICATION_SETTINGS.hashCode());
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            Hack.getStoragePath();
        }
    }

    final private String[] permissionsToRequest = {
            Manifest.permission.READ_CONTACTS,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            //Manifest.permission.MANAGE_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA,
            Manifest.permission.READ_PHONE_STATE,
            Manifest.permission.CALL_PHONE,
            Manifest.permission.RECORD_AUDIO,
    };
    List<String> mPermissionsNeeded = new ArrayList<>();
    private static String getPermissionName(String permission) {
        String[] parts = permission.split("\\.");
        if (parts.length > 0) {
            String lastPart = parts[parts.length - 1];
            String[] subParts = lastPart.split("_");
            StringBuilder permissionName = new StringBuilder();
            for (String subPart : subParts) {
                if (subPart.length() > 0) {
                    // Convert first letter to uppercase and the rest to lowercase
                    String formattedPart = subPart.substring(0, 1).toUpperCase() +
                            subPart.substring(1).toLowerCase();
                    permissionName.append(formattedPart).append(" ");
                }
            }
            return permissionName.toString().trim();
        }
        return null;
    }


    private void checkAndRequestPermissions() {

        StringBuilder permissionsText = new StringBuilder(getString(R.string.not_granted_permission)+"\n");
        mPermissionsNeeded.clear();
        for (String permission : permissionsToRequest) {
            if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                //if (ActivityCompat.shouldShowRequestPermissionRationale(this, permission)) {
                    mPermissionsNeeded.add(permission);
                    permissionsText.append(getPermissionName(permission)).append("\n");
                //}
            }
        }

        if (!mPermissionsNeeded.isEmpty()) {
            TextView permissionTextView = (TextView) findViewById(R.id.permissionMessageTextView);
            permissionTextView.setText(permissionsText.toString());
        } else {
            Log.d(DEBUGTAG,"All permissions are granted and finish.");
            finish();
        }
    }

    public void ClickHandler(View view) {
        if(view.getId()==R.id.requestPermissionButton) {
            Log.d(DEBUGTAG, "requestPermissions:"+mPermissionsNeeded.size());
            ActivityCompat.requestPermissions(
                    this,
                    mPermissionsNeeded.toArray(new String[0]),
                    REQUEST_CODE_PERMISSIONS
            );
            Log.d(DEBUGTAG,"start system requestPermissions activity and finish.");
            finish();
        }
    }
}
