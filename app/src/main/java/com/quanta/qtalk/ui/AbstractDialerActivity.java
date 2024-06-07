package com.quanta.qtalk.ui;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
//import android.os.Handler;
import android.widget.Toast;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.ui.contact.ContactManager;
import com.quanta.qtalk.util.Log;

public abstract class AbstractDialerActivity extends AbstractUiActivity 
{
//    private static final String TAG = "AbstractDialerActivity";
    private static final String DEBUGTAG = "QTFa_AbstractDialerActivity";
 //   private static final Handler mHandler = new Handler();
    private AudioManager mAudioManager = null;
    
    protected String mRemoteID = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        Uri uri = getIntent().getData();
        if (uri!=null)
        {
//            if (uri.getScheme().equals("sip"))
//                mRemoteID = uri.getSchemeSpecificPart();
//            else 
//            {
                if (uri.getAuthority().equalsIgnoreCase(ContactManager.CUSTOM_PROTOCOL))
                {
                    mRemoteID = uri.getLastPathSegment();
                }
//            }
        }else
        {
            Bundle bundle = this.getIntent().getExtras();
            if (bundle!=null)
            {
                 mRemoteID = bundle.getString(QTReceiver.KEY_REMOTE_ID);
            }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"@onCreate:"+uri+" ==> "+mRemoteID);
    }
    @Override
    protected void onStart() 
    {
        super.onStart();
        if (mRemoteID!=null)
        {
            makeCall(mRemoteID,true);
            this.finish();
        }
    }
    @Override
    protected void onResume() 
    {
        super.onResume();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
//        try
//        {
//            if (!QTReceiver.engine(this).isLogon())
//                QTReceiver.engine(this).login(this);
//        } catch (FailedOperateException e)
//        {
//            Log.e(DEBUGTAG,"login",e);
//        }
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (mAudioManager != null)
    		mAudioManager = null;
    }
    @Override
    protected void onPause() 
    {
        super.onPause();
        mRemoteID = null;
    }
    protected void exitQT()
    {
        QTReceiver.finishQT(this);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "Reprovision Flag is :"+Flag.Reprovision.getState());
       /* if (Flag.Reprovision.getState() == false)
        	RingingUtil.playSoundLogout(this);*/
    }
    
    @SuppressLint("ShowToast")
	protected synchronized boolean makeCall(String remoteID,boolean enableVideo)
    {
        boolean result = false;
        if ((mRemoteID==null && remoteID!=null && remoteID.trim().length() > 0) || (mRemoteID!=null && remoteID!=null && remoteID.compareToIgnoreCase(mRemoteID)==0))
        {
            //Use call engine to Make call
            mRemoteID = remoteID;
            try {
                QTReceiver.engine(this,false).makeCall(null,remoteID, enableVideo);
            } catch (FailedOperateException e) {
                Log.e(DEBUGTAG,"makeCall",e);
                Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
            }
        }
        else if(!(mRemoteID!=null && remoteID!=null && remoteID.compareToIgnoreCase(mRemoteID)==0))
        {
            Intent intent = new Intent();
            intent.setAction(QTReceiver.ACTION_HISTORY);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            
            // start Activity Class
            startActivity(intent);
            finish();
        }
        return result;
    }
	public void updateCallAvatar() {
		// TODO Auto-generated method stub
		
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
