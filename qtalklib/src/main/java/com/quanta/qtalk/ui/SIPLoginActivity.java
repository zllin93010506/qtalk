package com.quanta.qtalk.ui;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.LinearLayout;
//import android.widget.Toast;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.VersionDefinition;
import com.quanta.qtalk.util.Log;

public class SIPLoginActivity extends AbstractSetupActivity
{
//    private static final String TAG = "SIPLoginActivity";
    private LinearLayout mFullSipLogin = null;
	private static final String DEBUGTAG = "QTFa_SIPLoginActivity";
    private Handler mHandler = new Handler()
    {
        public void handleMessage(Message msg)
        {
            switch(msg.what)
            {
                case LOGIN_FAILURE:
                	//------------------------------------------------------------[PT]
			          if(QtalkSettings.Login_by_launcher==true){
			            Intent broadcast_intent = new Intent();
		                broadcast_intent.setAction("android.intent.action.vc.login.cb");
		 		        Bundle mybundle = new Bundle();
		 		        mybundle.putString("KEY_STATE" , "FAIL");
				 		mybundle.putString("KEY_REASON", "SIP LOGIN FAIL");
				 		broadcast_intent.putExtras(mybundle);
		 		        Log.d("SEND","Send Broadcast to Launcher login FAIL");
		 		        sendBroadcast(broadcast_intent);
			          }
	 		        //------------------------------------------------------------[PT]
                    if(VersionDefinition.CURRENT == VersionDefinition.PROVISION)
                    {
                    	if (ProvisionSetupActivity.debugMode)  Log.e(DEBUGTAG,"Login SIP failed!!");
                        Intent intent = new Intent();
                        intent.setClass(SIPLoginActivity.this, LoginRetryActivity.class);
                        SIPLoginActivity.this.startActivity(intent);
                        finish();
                    }
                    else
                    {
                    	if (ProvisionSetupActivity.debugMode)  Log.e(DEBUGTAG,"Login SIP failed!!");
                        Bundle bundle = msg.getData();
                        bundle.getString(ProvisionSetupActivity.MESSAGE);
                        Intent intent = new Intent();
                        intent.putExtras(bundle);
                        intent.setClass(SIPLoginActivity.this, LoginRetryActivity.class);
                        SIPLoginActivity.this.startActivity(intent);
                        finish();
                    }
                    finish();  
                    break;
                case LOGIN_SUCCESS:
                	if (ProvisionSetupActivity.debugMode)  Log.e(DEBUGTAG,"Login successful.");
                	if (Flag.Activation.getState()==true)
                	{
                		LogonStateReceiver.notifyLogonState(getApplicationContext(), true, null, "PTVC");
                		if(QtalkSettings.Login_by_launcher==true){
                			//------------------------------------------------------------[PT]
    	                    Intent broadcast_intent = new Intent();
    	                    broadcast_intent.setAction("android.intent.action.vc.login.cb");
    	    		        Bundle bundle = new Bundle();
    	    		        bundle.putString("KEY_LOGIN_CB" , "SUCCESS");
    	    		        broadcast_intent.putExtras(bundle);
    	    		        Log.d("SEND","Send Broadcast to Launcher SUCCESS");
    	    		        sendBroadcast(broadcast_intent);
    	    		        //------------------------------------------------------------[PT]
                			finish();
                		}else{
	                    Intent intent = new Intent();
	                   //intent.setClass(SIPLoginActivity.this, DialerActivity.class);
	                    intent.setClass(SIPLoginActivity.this, ContactsActivity.class);
	                   // intent.setClass(SIPLoginActivity.this, HistoryActivity.class);
	//                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
	//                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);                  
	                    SIPLoginActivity.this.startActivity(intent);
	                    
	    		        //RingingUtil.playSoundLogin(SIPLoginActivity.this);
	                    finish();
                		}
                	}
                    break;
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
      //  if (Flag.Reprovision.getState() == false)
        	setContentView(R.layout.layout_sip_login);//
        	mFullSipLogin = (LinearLayout) findViewById(R.id.mFullSipLogin);
        	if(QtalkSettings.Login_by_launcher==true){
        		mFullSipLogin.setVisibility(View.INVISIBLE);
        		//mFullSipLogin.setVisibility(Color.RED);
        	}
      /*  else
        {
        	setContentView(R.layout.layout_sip_login_transfer);
        	Flag.Reprovision.setState(false);
        }*/
    } 
    
    protected void onDestroy() 
    { 
        super.onDestroy();
        if(mFullSipLogin!=null)
        	mFullSipLogin.removeAllViews();
    }
    
    @Override
    public void onPause()
    {
        super.onPause();
    }
    @Override
    public void onResume()
    {
        super.onResume();
        if(QThProvisionUtility.provision_info == null)	
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "QThProvisionUtility.provision_info is null!!");
			Intent ProvisionIntent = new Intent();
	  		ProvisionIntent.setClass(SIPLoginActivity.this, ProvisionLoginActivity.class);
	  		startActivity(ProvisionIntent);
	  		finish();
        }
        else
        {
	        Log.d(DEBUGTAG, "onResume");
	        //TODO
	        //if no setting available--> to Loginretry or setting
	        try {
	            QtalkSettings settings = SIPLoginActivity.super.getSetting(false);
	            if (settings.proxy.trim().length()==0 ||
	                    settings.outBoundProxy.trim().length()==0 ||
	                    settings.userID.trim().length()==0 ||
	                    settings.authenticationID.trim().length()==0 ||
	                    settings.password.trim().length()==0 ||
	                    //settings.RealmServer.trim().length()==0 ||
	                    settings.port== 0||
	                    settings.expireTime== 0||
	                    settings.minVideoPort== 0||
	                    settings.maxVideoPort== 0||
	                    settings.minAudioPort== 0||
	                    settings.maxAudioPort== 0)
	            {
	                Intent intent = new Intent();
	                if (ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "Something of provision neccessary data is 0.");
	                intent.setClass(SIPLoginActivity.this, LoginRetryActivity.class);
	                startActivity(intent);
	                finish();
	                return;
	            }
	            
	        } catch (FailedOperateException e) {
	            Log.e(DEBUGTAG,"onCreate",e);
	          //  Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
	        }
	        
/*	        if(QTReceiver.engine(SIPLoginActivity.this,true).isLogon() == false)
	        {
	
	        }*/
	        new Thread( new Runnable(){
	            @Override
	            public void run()
	            {
	            	if(QTReceiver.engine(SIPLoginActivity.this,true).isLogon() == false)
	            	{
	            		if (ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "Prepare to SIP login");
		                Message msg = new Message(); 
		                Bundle bundle = new Bundle();
		                //Waiting for logon signal
		                for (int i=0;i<20;i++)
		                {
		                    if (QTReceiver.engine(SIPLoginActivity.this,false).isLogon())
		                        break;
		                    try
		                    {
		                        Thread.sleep(300);//1000
		                        if (i==19)
		                        	if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "SIP time out");
		                    } catch (InterruptedException e)
		                    {
		                    	Log.e(DEBUGTAG,"onResume",e);
		                    }
		                }
		                
		                if (QTReceiver.engine(SIPLoginActivity.this,false).isLogon())
		                {
		                	if (ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "SIP Login successfully");
		                    mHandler.sendEmptyMessage(LOGIN_SUCCESS);
		                }
		                else
		                {
		                	if (ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "SIP Login failed");
		                    bundle.putString(ProvisionSetupActivity.MESSAGE, "1網路忙碌中\r\n將在十秒後自動重試連線\r\n");
		                    msg.setData(bundle);
		                    msg.what = LOGIN_FAILURE;
		                    mHandler.sendMessage(msg);
		                }
	            	}
	            	else
	            	{
	            		mHandler.sendEmptyMessage(LOGIN_SUCCESS);
	            	}
	            }
	        }).start();
        }
    }
}

