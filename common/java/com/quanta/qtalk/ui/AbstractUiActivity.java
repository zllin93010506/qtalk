package com.quanta.qtalk.ui;

import com.quanta.qtalk.SR2_OP_Code;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;

public abstract class AbstractUiActivity extends Activity
{
	protected ServiceConnection   			m_connection_broadcast;
	protected ISmartConnectService     		m_service_broadcast;
	protected ISmartConnectCallback 			m_callback_broadcast;
	private Handler mSmartConnectHandler;
	protected LayoutInflater factory;
	protected AlertDialog.Builder DialogBuilder;
	protected AlertDialog EnterPinDialog;
//	private static final String TAG = "AbstractUiActivity";
	protected static final String DEBUGTAG = "QTFa_AbstractUiActivity";
	protected static final int BUSY = 0;
	protected static final int FAIL = 0;
	protected static final int LOGIN_FAILURE = 0;
	protected static final int LOGIN_SUCCESS = 1;
	protected static final int REPROVISION = 1;
	protected static final int AvatarUpdate = 2;
	protected static final int DEACTIVATE = 6;
	protected static final int AVATAR_SUCCESS = 7;
	protected static final int AVATAR_FAIL = 8;
	//protected Handler mSmartConnectResult = new Handler(Looper.getMainLooper());
    @Override
    protected void onResume()
    {
        super.onResume();
        //ViroActivityManager.setActivity(this);
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
    protected void
    _InitService(Handler mSmartConnectHandler_)
    {
    	mSmartConnectHandler = mSmartConnectHandler_;
    	m_connection_broadcast = new ServiceConnection() {
            public void 
            onServiceDisconnected(ComponentName className) 
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onServiceDisconnected");
                m_service_broadcast = null;
            }
			@Override
			public void onServiceConnected(ComponentName name, IBinder service) {
				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onServiceConnected");
				m_service_broadcast = ISmartConnectService.Stub.asInterface(service);
				m_callback_broadcast = new ISmartConnectCallback.Stub() {
					@Override
					public void FinishScanning() throws RemoteException {
						if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"FinishScanning()");
						Message message = new Message();
						message.what = SR2_OP_Code.OP_CODE_SMARTCONNECT_SCANNING_FINISH;
						if( mSmartConnectHandler != null) mSmartConnectHandler.sendMessage(message);
						
						if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"SR2SwitchHandler:"+mSmartConnectHandler);
					}

					@Override
					public void returnResult(SR2SmartConnectData data) throws RemoteException {
						if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"IP:"+data.getIP()+ " Port:"+data.getPort()+" MAC:"+data.getMAC()+" DISPLAY_NAME:"+data.getDISPLAY_NAME()+" SIP_ID:"+data.getSIP_ID());
						Message message = new Message();
						message.obj = data;
						message.what = SR2_OP_Code.OP_CODE_SMARTCONNECT_RESULT;
						if( mSmartConnectHandler != null) mSmartConnectHandler.sendMessage(message);
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
	  if (keyCode == KeyEvent.KEYCODE_BACK ) 
	  {
    	Intent PhoneHome = new Intent(Intent.ACTION_MAIN);  
    	PhoneHome.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);  
    	PhoneHome.addCategory(Intent.CATEGORY_HOME);  
    	startActivity(PhoneHome);
	  }
	  return super.onKeyDown(keyCode, event);
	 }
    
/*    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
    	//Log.d(TAG,"onKeyDown");
        if (keyCode == KeyEvent.KEYCODE_BACK ) 
        {
        	Intent PhoneHome = new Intent(Intent.ACTION_MAIN);  
        	PhoneHome.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);  
        	PhoneHome.addCategory(Intent.CATEGORY_HOME);  
        	startActivity(PhoneHome);
        }
        return super.onKeyDown(keyCode, event);
    }*/
}
