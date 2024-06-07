package com.quanta.qtalk.ui;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.util.Log;

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.widget.Toast;

public class AbstractCallTimerActivity extends AbstractBaseCallActivity
{
    private Handler mHandlerTime = new Handler();
    private static final String DEBUGTAG = "QTFa_AbstractCallTimerActivity";
    private AudioManager mAudioManager = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        mHandlerTime.postDelayed(timerRun, 180000);
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        mHandlerTime.removeCallbacks(timerRun);
        if (mAudioManager != null)
    		mAudioManager = null;
    }
    @Override
    protected void onResume()
    {
        super.onResume();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
    }
    @SuppressLint("ShowToast")
	protected boolean hangup()
    {
    	Log.d("Frank", "AbstractCallTimerActivity.hangup(), mCallID:" + mCallID + ", mRemoteID:" + mRemoteID);
        AppStatus.setStatus(AppStatus.IDEL);
    	boolean result = false;
        try {
            if (mCallID != null)
                QTReceiver.engine(this,false).hangupCall(mCallID, mRemoteID, false);
            RingingUtil.stop();
            RingingUtil.playSoundHangup(this);
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"hangup",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
        return result;
    }
    
    private final Runnable timerRun = new Runnable()
    {
      public void run()
      {
        hangup();
        finish();
      }
    };
    
//    @Override
//	 public boolean onKeyDown(int keyCode, KeyEvent event) {
//	  switch (keyCode) {
//	  case KeyEvent.KEYCODE_VOLUME_UP:
//		  mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC,
//		  AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
//	      return true;
//	  case KeyEvent.KEYCODE_VOLUME_DOWN:
//		  mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC,
//	      AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
//	      return true;
//	  default:
//	   break;
//	  }
//	  return super.onKeyDown(keyCode, event);
//	 }
}
