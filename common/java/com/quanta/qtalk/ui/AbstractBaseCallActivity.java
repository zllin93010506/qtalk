package com.quanta.qtalk.ui;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
//import android.content.IntentFilter;
import android.net.ConnectivityManager;
//import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.WindowManager;

import com.quanta.qtalk.ui.contact.ContactManager;
import com.quanta.qtalk.ui.contact.ContactQT;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public abstract class AbstractBaseCallActivity extends AbstractUiActivity {
    protected String   mRemoteID = null;
    protected String   mCallID = null;
    protected ContactQT mContactQT = null;
    protected boolean  mEnableVideo = true;
    protected final Handler mHandler = new Handler();
//    private static final String TAG = "AbstractBaseCallActivity";
    private static final String DEBUGTAG = "QTFa_AbstractBaseCallActivity";
    protected Dialog mDialog = null;

    protected String AC_VC_HANGON = "android.intent.action.vc.hangon";
    protected String AC_VC_HANGUP = "android.intent.action.vc.hangup";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
    	
        super.onCreate(savedInstanceState);
        // Set keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (savedInstanceState!=null)
        {
            mRemoteID    = savedInstanceState.getString(QTReceiver.KEY_REMOTE_ID);
            mCallID      = savedInstanceState.getString(QTReceiver.KEY_CALL_ID);
            mEnableVideo = savedInstanceState.getBoolean(QTReceiver.KEY_ENABLE_VIDEO);

            QTReceiver.registerActivity(mCallID, this);
            
        }else
        {
            Bundle bundle = this.getIntent().getExtras();
            if (bundle!=null)
            {
                mRemoteID    = bundle.getString(QTReceiver.KEY_REMOTE_ID);
                mCallID      = bundle.getString(QTReceiver.KEY_CALL_ID);
                mEnableVideo = bundle.getBoolean(QTReceiver.KEY_ENABLE_VIDEO);
                QTReceiver.registerActivity(mCallID, this);
            }
        }
        if (mRemoteID!=null)
            mContactQT = ContactManager.getContactByQtAccount(this, mRemoteID);

        //-------------------[PT]
        CommandService.mCallID = mCallID;
        CommandService.mRemoteID = mRemoteID;

    }
    
    protected void sendHangOnBroadcast()
    {
        //send hang on broadcast
        Intent intent = new Intent();
        intent.setAction(AC_VC_HANGON);
        sendBroadcast(intent);
        Log.d(DEBUGTAG, "send "+AC_VC_HANGON);
    }

    protected void sendHangUpBroadcast()
    {
        //send hang on broadcast
        Intent intent = new Intent();
        intent.setAction(AC_VC_HANGUP);
        sendBroadcast(intent);
        Log.d(DEBUGTAG, "send "+AC_VC_HANGUP);
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        
        QTReceiver.unregisterActivity(mCallID, this);
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    @Override
    protected void onSaveInstanceState (Bundle outState)
    {
        super.onSaveInstanceState(outState);
        if (outState!=null)
        {
            outState.putString(QTReceiver.KEY_REMOTE_ID, mRemoteID);
            outState.putString(QTReceiver.KEY_CALL_ID, mCallID);
            outState.putBoolean(QTReceiver.KEY_ENABLE_VIDEO, mEnableVideo);
        }
    }
    
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
    	//Log.d(TAG,"onKeyDown");
        if (keyCode == KeyEvent.KEYCODE_BACK ) 
        {
           // Toast.makeText(this, "onKeyDown:KEYCODE_BACK", Toast.LENGTH_SHORT);
           return true;
        }else if (keyCode == KeyEvent.KEYCODE_MENU)
        {
            //Toast.makeText(this, "onKeyDown:KEYCODE_MENU", Toast.LENGTH_SHORT);
            return super.onKeyDown(keyCode, event);
        }
        return super.onKeyDown(keyCode, event);
    }
}
