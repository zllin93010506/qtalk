package com.quanta.qtalk.ui;

import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.view.KeyEvent;

public class AbstractVolumeActivity extends Activity{
	
	private static final String DEBUGTAG = "QTFa_AbstractVolumeActivity";
	private AudioManager mAudioManager = null;
	
	 @Override
	 protected void onResume()
	 {
		 super.onResume();
		 //if ((this.getResources().getConfiguration().screenLayout &      Configuration.SCREENLAYOUT_SIZE_MASK) > Configuration.SCREENLAYOUT_SIZE_LARGE)
         if(!Hack.isPhone())
		 {//Detect Android Table => Landscape
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
         }else
         {//
            if(AbstractInVideoSessionActivity.class.isInstance(this))
            {
                return;
            }
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
         }
		 mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
	 }
	 @Override
	 protected void onDestroy()
	 {
		 super.onDestroy();
		 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onDestroy");
		 if (mAudioManager != null)
			 mAudioManager = null;
	 }
	 @Override
	 public boolean onKeyDown(int keyCode, KeyEvent event) {
		 switch (keyCode) {
		 case KeyEvent.KEYCODE_VOLUME_UP:
			 mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC,
					 AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
			 return true;
		 case KeyEvent.KEYCODE_VOLUME_DOWN:
			 mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC,
					 AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
			 return true;
		 default:
			 break;
		 }
		 return super.onKeyDown(keyCode, event);
	 }
}
