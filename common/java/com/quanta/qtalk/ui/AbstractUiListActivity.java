package com.quanta.qtalk.ui;

import com.quanta.qtalk.QtalkDB;

import android.app.ExpandableListActivity;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.RemoteException;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import android.view.KeyEvent;

public abstract class AbstractUiListActivity extends ListActivity
{
	protected ServiceConnection   			m_connection_broadcast;
	protected ISmartConnectService     		m_service_broadcast;
	protected ISmartConnectCallback 			m_callback_broadcast;
	private AudioManager mAudioManager = null;
	private static final String DEBUGTAG = "QTFa_AbstractUiListActivity";
	protected static final int BUSY = 0;
	protected static final int FAIL = 0;
    @Override protected void onResume()
    {
        super.onResume();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
       // ViroActivityManager.setActivity(this);
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
    }
    protected void onStart()
    {
    	super.onStart();
    }
    protected void onStop()
    {
    	super.onStop();
    }
    protected void onDestory()
    {
    	super.onDestroy();
    	if (mAudioManager != null)
    		mAudioManager = null;
    }
    protected void _InitService()
    {
    	m_connection_broadcast = new ServiceConnection() {
            public void 
            onServiceDisconnected(ComponentName className) 
            {
                m_service_broadcast = null;
            }
			@Override
			public void onServiceConnected(ComponentName name, IBinder service) {
				m_service_broadcast = ISmartConnectService.Stub.asInterface(service);
				m_callback_broadcast = new ISmartConnectCallback.Stub() {

					@Override
					public void FinishScanning() throws RemoteException {
						// TODO Auto-generated method stub
						
					}

					@Override
					public void returnResult(SR2SmartConnectData data)
							throws RemoteException {
						// TODO Auto-generated method stub
						
					}


		        };
		  
		    	try {
		    		m_service_broadcast.RegisterCallback(m_callback_broadcast);
		        } catch (RemoteException e) {
		        	Log.e(DEBUGTAG,"onServiceConnected",e);
		        }
		        
			}
        };
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
 	
 	  if (keyCode == KeyEvent.KEYCODE_BACK ) 
     {
     	Intent PhoneHome = new Intent(Intent.ACTION_MAIN);  
     	PhoneHome.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);  
     	PhoneHome.addCategory(Intent.CATEGORY_HOME);  
     	startActivity(PhoneHome);
     }
	  return super.onKeyDown(keyCode, event);
	 }
}
