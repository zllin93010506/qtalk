package com.quanta.qtalk.ui;

import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Log;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.view.WindowManager;

public class TransparentActivity extends Activity {

	private static final String DEBUGTAG = "QSE_TransparentActivity";
	private WakeLock mWakeLock = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		PermissionRequestActivity.requestPermission(this);
		setContentView(R.layout.activity_transparent);
	}

	@SuppressWarnings("deprecation")
	@Override
	public void onResume() {
		super.onResume();
		Log.d(DEBUGTAG, "onResume");

		Intent ri = getIntent();
		if (ri == null) {
			Log.w(DEBUGTAG, "no intent");
			finishAffinity();
			return;
		}
		Bundle b = ri.getExtras();
		if (b == null) {
			Log.e(DEBUGTAG, "no boudles");
			finishAffinity();
			return;
		}
		Intent i = new Intent();
		i.setComponent(new ComponentName("com.quanta.qtalk", "com.quanta.qtalk.ui.CommandService"));
		i.putExtras(b);
		startService(i);
		Log.d(DEBUGTAG, "start myservice");
		finishAffinity();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		Log.d(DEBUGTAG, "onDestroy");
	}
}
