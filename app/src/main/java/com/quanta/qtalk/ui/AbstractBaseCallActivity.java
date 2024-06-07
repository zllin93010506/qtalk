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
    protected Dialog mDTMFDialog = null;
	protected AlertDialog mAlertDialog = null;
    protected static final int ALERT_FADING_TIME = 1500; //milli second

    protected String AC_VC_HANGON = "android.intent.action.vc.hangon";
    protected String AC_VC_HANGUP = "android.intent.action.vc.hangup";
    
    BroadcastReceiver wifi_connect_action_br = new BroadcastReceiver(){
		public void onReceive(final Context context,final Intent intent)
		{
			String intentAction = intent.getAction();
			if (intentAction.equals(ConnectivityManager.CONNECTIVITY_ACTION))
	        {
	        	ConnectivityManager connManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE); 
	        	NetworkInfo info=connManager.getActiveNetworkInfo();
	        	
	        	if(info != null)
	        	{
	        		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Type:"+info.getTypeName()+" TypeNum:"+info.getType()+ " State:"+info.getDetailedState()+" describeContents:"+info.describeContents()+" SubType:"+info.getSubtypeName());
	        		//Log.d(TAG,"getLocalIpAddress:"+getLocalIpAddress());
	        		WifiManager wifi_service = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
	        		WifiInfo wifiinfo = wifi_service.getConnectionInfo();
	        		
	        		SupplicantState sstate = wifiinfo.getSupplicantState();
	        		WifiInfo.getDetailedStateOf(sstate);
	        		//DhcpInfo dhcpinfo = wifi_service.getDhcpInfo();

		        	if(info.getType() == ConnectivityManager.TYPE_WIFI/* && !prev_ip.equals(ntohl(dhcpinfo.ipAddress))*/)
		        	{
		        		
		        		//prev_ip = new String(ntohl(dhcpinfo.ipAddress));
		        	}
	        		
	        	}

	        }
		}
	};
    
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

    protected synchronized void buildDialog(final Dialog dialog)
    {
//    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>buildDialog");
        cleanAlertDialog();
        cleanDialog();
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
                    if(dialog != null)
                    {
                        mDialog = dialog;
                        mDialog.show();
                        new Thread(new Runnable(){
                            @Override
                            public void run()
                            {
                                try{
                                    if(Hack.isMotorolaAtrix())
                                        Thread.sleep(ALERT_FADING_TIME+500);
                                    else
                                        Thread.sleep(ALERT_FADING_TIME);
                                    cleanDialog();
                                }
                                catch(Exception e)
                                {
                                    Log.e(DEBUGTAG,"error",e);
                                }
                            }
                        }).start();
                    }
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==buildDialog");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"buildDialog",err);
                }
            }
        });
    }
    
    protected synchronized void buildAlertDialog(final AlertDialog alertDialog)
    {
//    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>buildAlertDialog");
        cleanAlertDialog();
        cleanDialog();
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
                    if(alertDialog != null)
                    {
                        mAlertDialog = alertDialog;
                        mAlertDialog.show();
                    }
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==buildAlertDialog");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"buildAlertDialog",err);
                }
            }
        });
    }
    
    protected synchronized void cleanDialog()
    {
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
//                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>cleanDialog");
                    if(mDialog != null)
                    {
                        mDialog.dismiss();
                        mDialog = null;
                    }
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==cleanDialog");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"cleanDialog",err);
                }
            }
        });
    }
    
    protected synchronized void cleanAlertDialog()
    {
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
//                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>cleanAlertDialog");
                    if(mAlertDialog != null)
                    {
                        mAlertDialog.dismiss();
                        mAlertDialog = null;
                    }
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==cleanAlertDialog");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"cleanAlertDialog",err);
                }
            }
        });
    }
    protected synchronized void createMessage(final String message, final boolean isDismiss)
    {
        mHandler.post(new Runnable()
        {
            public void run()
            {
                try
                {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(AbstractBaseCallActivity.this);
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>createMesage");
                    if(mAlertDialog != null && mAlertDialog.isShowing())
                    {
                        mAlertDialog.dismiss();
                        mAlertDialog = null;
                    }
                    if(builder != null)
                    {
                        mAlertDialog = builder.setMessage(message)
                                              .setCancelable(true)
                                              .create();
                    }
                    if(mAlertDialog != null)
                        mAlertDialog.show();
                    if(isDismiss)
                    {
                        new Thread(new Runnable(){
                            @Override
                            public void run()
                            {
                                try
                                {
                                    Thread.sleep(1000);
                                    mAlertDialog.dismiss();
                                    mAlertDialog = null;
                                } catch (Exception e)
                                {
                                    Log.e(DEBUGTAG,"error",e);
                                }
                            }
                        }).start();
                    }
//                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==createMesage");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"createMesage",err);
                }
            }
        });
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        
        QTReceiver.unregisterActivity(mCallID, this);
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        cleanAlertDialog();
        cleanDialog();

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
    
 /*   public synchronized void finishWithDialog(final String message)
    {
        finishWithDialog(message, true);
    }
    */
  /*  public synchronized void finishWithDialog(final String message, final boolean playHaungTone)
    {
        cleanDialog();
        cleanAlertDialog();
        Log.d(DEBUGTAG, "finishWithDialog:"+message);
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
                    if(playHaungTone)
                        RingingUtil.playSoundHangup(AbstractBaseCallActivity.this);
                    //Toast remote hangup
                    AlertDialog.Builder builder = new AlertDialog.Builder(AbstractBaseCallActivity.this);
                    builder.setMessage(message);
                    if(builder != null)
                        mAlertDialog = builder.create();
                    if(mAlertDialog != null)
                        mAlertDialog.show();
                    new Thread(new Runnable(){
                        @Override
                        public void run()
                        {
                            try
                            {
                                Thread.sleep(1000);
                                cleanAlertDialog();
                                AbstractBaseCallActivity.this.finish();
                            } catch (Exception e)
                            {
                                Log.e(DEBUGTAG,"",e);
                            }
                        }
                    }).start();
                }catch(Exception ignore)
                {
                    Log.e(DEBUGTAG,"finishWithDialog",ignore);
                }
            }
        });
    }
    */
  /*  protected void qth_switch_call_to_px(String PX_SIP_ID)
    {
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
                QTReceiver.engine(this,false).qth_switch_call_to_px(mCallID, PX_SIP_ID);
            }
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"setHold",e);
        }
    }*/
}
