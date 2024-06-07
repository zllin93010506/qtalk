package com.quanta.qtalk.ui;


import android.annotation.SuppressLint;
//import android.app.Activity;
import android.content.Context;
import android.content.Intent;
//import android.graphics.PixelFormat;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.PowerManager;

import com.quanta.qtalk.AppStatus;
//import com.quanta.qtalk.R;
import com.quanta.qtalk.call.ICallListener;
import com.quanta.qtalk.util.Log;

//import android.view.Gravity;
import android.view.LayoutInflater;
//import android.view.Window;
//import android.view.WindowManager;
//import android.view.WindowManager.LayoutParams;
import android.widget.LinearLayout;
import android.widget.Toast;
import android.os.PowerManager.WakeLock;

public abstract class AbstractIncomingCallActivity extends AbstractCallTimerActivity  implements ICallListener
{
    private static final String DEBUGTAG = "QTFa_AbstractIncomingCallActivity";
    private boolean mIsAccepted = false;
    private WakeLock mWakeLock = null;
    LinearLayout mIncomingcallLayout=null;
    LayoutInflater li=null;

    @SuppressWarnings("deprecation")
	@Override
    protected void onStart()
    {
        super.onStart();
//        this.setVolumeControlStream(AudioManager.STREAM_RING);
        
        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK
                |PowerManager.ACQUIRE_CAUSES_WAKEUP, "qtalk:AbstractIncomingCallActivity");
        mWakeLock.acquire();
        takeKeyEvents(true);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        boolean success = QTReceiver.engine(this,false).setCallListener(mCallID, this);
        if (success) {
            //RingingUtil.playSoundIncomingCall(this);
			Bundle bundle = new Bundle();
			bundle.putInt("KEY_COMMAND", CommandService.command_playsound);
			Intent myIntent = new Intent(this, CommandService.class);
			myIntent.putExtras(bundle);
			this.startForegroundService(myIntent);
        }
        else {
            this.finish();
        }
    }
    
    @Override
    protected void onStop()
    {
    	super.onStop();
//        this.setVolumeControlStream(AudioManager.USE_DEFAULT_STREAM_TYPE);
    }
    
    @Override
    protected void onPause()
    {
        super.onPause();
        RingingUtil.stop();
    }
    
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (!mIsAccepted)
        {
            try
            {
                onCallMissed(mRemoteID);
            }
            catch (Throwable e) {
                Log.e(DEBUGTAG,"onCallMissed",e);
            }
        }
        if (mWakeLock != null && mWakeLock.isHeld()) {
            mWakeLock.release();
        }        
    }
    
    @SuppressLint("ShowToast")
	protected boolean accept(boolean enableVideo, boolean enableCameraMute)
    {
    	AppStatus.setStatus(AppStatus.SESSION);
    	boolean result = false;
        try {
            RingingUtil.stop();
            
            //Use engine to accept call
            if (mCallID!=null)
            {
                mIsAccepted = true;
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "mCallID is not null");
                QTReceiver.engine(this,false).acceptCall(mCallID,mRemoteID, enableVideo, enableCameraMute);
            }
            else
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "mCallID is null");
                finish();
            }
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"accept",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
        return result;
    }
    
    protected abstract void onCallMissed(String remoteID);
}
