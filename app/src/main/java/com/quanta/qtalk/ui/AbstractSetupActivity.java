package com.quanta.qtalk.ui;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.AudioManager;
import android.os.Handler;

import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.util.Log;

import android.view.KeyEvent;
import android.widget.Toast;

public abstract class AbstractSetupActivity extends AbstractUiActivity{
//    private static final String TAG = "AbstractSetupActivity"; 
    private static final String DEBUGTAG = "QTFa_AbstractSetupActivity";
    private AudioManager mAudioManager = null;
    //protected static final Handler mHandler = new Handler(); 
    protected QtalkSettings getSetting(final boolean enableService) throws FailedOperateException
    {	
        //Read settings
        return QTReceiver.engine(this,enableService).getSettings(this);
    }
    
    protected void updateSetting(final ProvisionInfo provisionInfo) throws FailedOperateException
    {	
         QtalkEngine engine = QTReceiver.engine(AbstractSetupActivity.this,false);
         if (engine!=null)
         {
             try {
                 engine.updateSetting(AbstractSetupActivity.this, provisionInfo);
             }catch (FailedOperateException e) {
                 Log.e(DEBUGTAG,"updateSetting",e);
             }
         }
    }
    
    @SuppressLint("ShowToast")
	protected void updateSetting(final QtalkSettings settings,final boolean enableService) throws FailedOperateException
    {
//        mHandler.post(new Runnable()
//         {
//             public void run()
//             {
                 QtalkEngine engine = QTReceiver.engine(AbstractSetupActivity.this,enableService);
                 if (engine!=null)
                 {
                     try { 
                         //Write settings & re-login
                         engine.updateSetting(AbstractSetupActivity.this, settings);
                     }catch (FailedOperateException e) {
                         //QTReceiver.notifyLogonState(AbstractSetupActivity.this,false,engine.getLogonName(),e.getMessage());
                    	 
                    	 	 Log.e(DEBUGTAG,"updateSetting",e);
                         Toast.makeText(AbstractSetupActivity.this, e.toString(), Toast.LENGTH_LONG);
                     }
                 }
//             }
//        });
    }
    protected void exitQT()
    {	
        QTReceiver.finishQT(this);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"playSoundLogout");
        RingingUtil.playSoundLogout(this);
    }
    @Override
	 protected void onResume()
	 {
		 super.onResume();
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
