package com.quanta.qtalk.ui;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import android.content.Intent;
//import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.database.Cursor;
import android.graphics.Color;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkMain;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.provision.FileUtility;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.QThProvisionUtility;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Log;

public class ProvisionLoginActivity extends AbstractSetupActivity
{
	private static final String DEBUGTAG = "QSE_ProvisionLoginActivity";
    private static final int LOGIN_FAILURE          = 0;
    private static final int LOGIN_SUCCESS          = 1;
    private static final int DEACTIVATION_SUCCESS   = 6;
    private String versionNumber;
    private ProvisionInfo provision_info = null;
    private TextView versionName;
    private LinearLayout mFullLoginSetup = null;
    
    private Handler mHandler = new Handler()
    {
        public void handleMessage(Message msg)
        {
          switch(msg.what)
          {
              case LOGIN_FAILURE:
            	  Log.d(DEBUGTAG," LOGIN_FAILED");
                  Intent myintent = new Intent();
		          Bundle bundle = new Bundle();
		          boolean is_network = true;
		          if (msg.getData().getString("msg") != null && !msg.getData().getString("msg").contentEquals("busy"))
		          {
		        	  if (msg.getData().getString("msg").contentEquals("idfail"))
		        		  bundle.putString(ProvisionSetupActivity.MESSAGE, "帳號不存在\r\n");
		        	  if (msg.getData().getString("msg").contentEquals("confirmfail"))
		        		  bundle.putString(ProvisionSetupActivity.MESSAGE, "密碼錯誤\r\n");
		        	  myintent.putExtras(bundle);
		        	  myintent.setClass(ProvisionLoginActivity.this,com.quanta.qtalk.ui.ProvisionSetupActivity.class);
		          }
		          else
		          {
		        	  myintent.setClass(ProvisionLoginActivity.this,com.quanta.qtalk.ui.LoginRetryActivity.class);
		        	  is_network = false;
		          }
		          //------------------------------------------------------------[PT]
		          if(QtalkSettings.Login_by_launcher==true){
			            Intent broadcast_intent = new Intent();
		                broadcast_intent.setAction("android.intent.action.vc.login.cb");
		 		        Bundle mybundle = new Bundle();
		 		        mybundle.putString("KEY_LOGIN_CB" , "FAIL");
		 		        broadcast_intent.putExtras(mybundle);
		 		        Log.d("SEND","Send Broadcast to Launcher login FAIL");
		 		        sendBroadcast(broadcast_intent);
		          }
		          //------------------------------------------------------------[PT]
                  if(is_network || !(QtalkSettings.Login_by_launcher))
                	  ProvisionLoginActivity.this.startActivity(myintent);
                  finish();
                  break;                 
              case LOGIN_SUCCESS:
            	  Log.d(DEBUGTAG," LOGIN_SUCCESS");
            	  Flag.SIPRunnable.setState(false);
                  Intent intent = new Intent();
                  intent.putExtras(msg.getData());
                  intent.setClass(ProvisionLoginActivity.this,com.quanta.qtalk.ui.QtalkCheckActivity2.class);
                  ProvisionLoginActivity.this.startActivity(intent);
                  Flag.SessionKeyExist.setState(true);
                  finish();
                  break;   
                 
              case DEACTIVATION_SUCCESS:
            	  Log.d(DEBUGTAG,"The app is deactivation!!!");
                  Flag.SIPRunnable.setState(false);
		          myintent = new Intent();
                  myintent.setClass(ProvisionLoginActivity.this,com.quanta.qtalk.ui.QtalkCheckActivity2.class);
                  ProvisionLoginActivity.this.startActivity(myintent);
                  finish();
                  break;
                  
          }
        }
    };
    
    
  
    @Override
    public void onCreate(Bundle savedInstanceState){
    	Log.d(DEBUGTAG,"==>onCreate "+this.hashCode());
        super.onCreate(savedInstanceState);
    	setContentView(R.layout.layout_provision_login);
    	mFullLoginSetup = (LinearLayout) findViewById(R.id.FullLayoutSetup);
    	if(QtalkSettings.Login_by_launcher==true){
    		mFullLoginSetup.setVisibility(View.INVISIBLE);
    		//mFullLoginSetup.setBackgroundColor(Color.GRAY);
    	}
    	Log.d(DEBUGTAG,"<==onCreate "+this.hashCode());
    }
	
    
    protected void onDestroy() 
    { 
    	Log.d(DEBUGTAG,"==>onDestroy "+this.hashCode());
        super.onDestroy();
        if(mFullLoginSetup!=null)
        	mFullLoginSetup.removeAllViews();
        Log.d(DEBUGTAG,"<==onDestroy "+this.hashCode());
    }
    
    @Override
    public void onPause()
    {	
        super.onPause();
    }
    @Override
    public void onResume()
    {
    	Log.d(DEBUGTAG,"==>onResume "+this.hashCode());
        super.onResume();

        try {
            final QtalkSettings qtalk_settings = super.getSetting(false);            
            TextView mUserName = (TextView) findViewById(R.id.mUserName);
            TextView mPassWord = (TextView) findViewById(R.id.mPassWord);
            mUserName.setText(qtalk_settings.provisionID);
            mPassWord.setText(qtalk_settings.provisionPWD);
            
            versionName = (TextView) findViewById(R.id.versionName);
        	String version=null;
    		int versionCode=0;
    		try {
    			version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
    			versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
    			versionName.setText("QSPT-VC "+version);
    			versionNumber = version + "."+versionCode;
    		} catch (NameNotFoundException e1) {
    			Log.e(DEBUGTAG,"getVersion",e1);
    			Log.e(DEBUGTAG,"",e1);
    		}
    		
            new Thread( new Runnable(){
                @Override
                public void run()
                {
                	// for test purpose "/pod/dispatch.do" "/~kakasi/provision/provision.php"
                	QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);
                	QtalkDB qtalkdb = new QtalkDB();
                	
                	
                	Cursor cs = qtalkdb.returnAllATEntry();
                	int size = cs.getCount();
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"DB size="+size);

                	if (size > 0)
                	{
                		Flag.SessionKeyExist.setState(true);
                		String path = Hack.getStoragePath()+"provision.xml";
                		File file = new File (path);
    					if (file.exists())
    					{	
    						try {
								provision_info = ProvisionUtility.parseProvisionConfig(path);
							} catch (ParserConfigurationException e) {
								// TODO Auto-generated catch block
								Log.e(DEBUGTAG,"parseProvisionConfig",e);
							} catch (SAXException e) {
								// TODO Auto-generated catch block
								Log.e(DEBUGTAG,"parseProvisionConfig",e);
							} catch (IOException e) {
								Log.e(DEBUGTAG,"parseProvisionConfig",e);
							};
							qtalk_settings.provisionServer = provision_info.provision_uri;
    					}
                		cs.moveToFirst();
                		Log.d(DEBUGTAG,"release_test Flag.Activation= "+Flag.Activation.getState()+
                				"line="+Thread.currentThread().getStackTrace()[2].getLineNumber());
                		Flag.Activation.setState(true);
				        int cscnt = 0;
				        for( cscnt = 0; cscnt < cs.getCount(); cscnt++)
				        {
				        	qtalk_settings.session = cs.getString(cs.getColumnIndex("Session"));
				        	qtalk_settings.key = cs.getString(cs.getColumnIndex("Key"));
				        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Session:"+ cs.getString(cs.getColumnIndex("Session")));
				        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Key:"+ cs.getString(cs.getColumnIndex("Key")));
				        	cs.moveToNext();
				        }
                		String login_url = qtalk_settings.provisionServer+
                				qtalk_settings.provisionType+"&service=activate&uid="+qtalk_settings.provisionID+"&session="+qtalk_settings.session;
                		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"login_url:"+ login_url);
                		try {
    							qtnMessenger.httpsConnect(login_url,qtalk_settings.session,versionNumber);
                    	}
                        catch (IOException e) {
                        	Log.d(DEBUGTAG,e.getMessage());
                        	Message msg = new Message();
        		            msg.what = LOGIN_FAILURE;
        		            Bundle bundle = new Bundle();
        		            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
        		            msg.setData(bundle);
        		            mHandler.sendMessage(msg);
                        }
                	}
                	else
                	{
                		Flag.SessionKeyExist.setState(false);
                		Log.d(DEBUGTAG,"release_test Flag.Activation= "+Flag.Activation.getState()+
                				"line="+Thread.currentThread().getStackTrace()[2].getLineNumber());
                		Flag.Activation.setState(false);
                		String Storage = Hack.getStoragePath();
                		File file = new File (Storage);
                		if (file.exists())
                		{
                			FileUtility.delFolder(Storage);
                			Intent intent = new Intent();
                	        intent.setAction(LauncherService.PTA_OP_GET);
                	        sendBroadcast(intent);                			
                		}
                        
                		String provision_url = qtalk_settings.provisionServer+
                				qtalk_settings.provisionType+"&service=activate&uid="+qtalk_settings.provisionID+"&confirm="+qtalk_settings.provisionPWD;
                		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"provision_url:"+ provision_url);
                		try {
	                    	qtnMessenger.httpsConnect(provision_url,
	                    			qtalk_settings.provisionServer,
	                    			qtalk_settings.provisionID,
	                    			qtalk_settings.provisionType,
	                    			versionNumber);
                    	}
                        catch (IOException e) {
                        	Log.d(DEBUGTAG,e.getMessage());
                        	Message msg = new Message();
        		            msg.what = LOGIN_FAILURE;
        		            Bundle bundle = new Bundle();
        		            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
        		            msg.setData(bundle);
        		            mHandler.sendMessage(msg);
                        }
					}
                	cs.close();
			        qtalkdb.close();                	
                }
            }).start();
            
        } catch (FailedOperateException e) 
        {
            Log.e(DEBUGTAG,"onResume",e);
            Message msg = new Message();
            msg.what = LOGIN_FAILURE;
            Bundle bundle = new Bundle();
            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
            msg.setData(bundle);
            mHandler.sendMessage(msg);
        }
        Log.d(DEBUGTAG,"<==onResume "+this.hashCode());
    }
}
















