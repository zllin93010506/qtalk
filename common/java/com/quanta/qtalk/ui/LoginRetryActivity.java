package com.quanta.qtalk.ui;

import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkDB;
//import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.database.Cursor;
/*import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.database.Cursor;*/
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.TextView;

public class LoginRetryActivity extends Activity
{
    private TextView mTxtStatus = null;
    private ImageButton   mBtnSettings = null;
    String message = null;
//    private static final String TAG = "LoginRetryActivity";
    private static final String DEBUGTAG = "QTFa_LoginRetryActivity";
 /*   public static final int     CONFIGURE_MENU_ITEM=Menu.FIRST;
    public static final int     ADD_CONTACT_ITEM = CONFIGURE_MENU_ITEM + 1;
    public static final int     DIRECTORY_MENU_ITEM = CONFIGURE_MENU_ITEM + 2;
    public static final int     EXIT_MENU_ITEM = CONFIGURE_MENU_ITEM + 3;
    public static final int     SR2_MENU_ITEM = CONFIGURE_MENU_ITEM + 4;*/
    private Handler mHandler = new Handler();
    private int Clickable = 0;
    
    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setContentView(R.layout.layout_login_retry);//
        ViroActivityManager.setActivity(this);
        if(QTReceiver.engine(LoginRetryActivity.this, false).isLogon())
        {
        	QTReceiver.engine(LoginRetryActivity.this,false).logout("");
     	}
        //if ((this.getResources().getConfiguration().screenLayout &      Configuration.SCREENLAYOUT_SIZE_MASK) > Configuration.SCREENLAYOUT_SIZE_LARGE)
        if(!Hack.isPhone())
        {//Detect Android Table => Landscape
        
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }else
        {//
        
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }
        
        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);
    	QtalkDB qtalkdb = new QtalkDB();
    	
    	Cursor cs = qtalkdb.returnAllATEntry();
    	int size = cs.getCount();
    	if (size > 0)
    	{
    		Flag.SessionKeyExist.setState(true);
    	}
    	else
    	{
    		Flag.SessionKeyExist.setState(false);
    	}
    	cs.close();
    	qtalkdb.close();
        mTxtStatus = (TextView) findViewById(R.id.mTxtStatus);
        mBtnSettings=(ImageButton)findViewById(R.id.mBtnSettings);
        //mBtnSignIn=(ImageButton)findViewById(R.id.mBtnSignIn);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onCreate:"+QTReceiver.engine(this, false).isLogon());
        
        Bundle bundle = this.getIntent().getExtras();
        if(bundle != null)
        {
            message = bundle.getString(ProvisionSetupActivity.MESSAGE);
        }
        if(mTxtStatus!= null)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"message:"+message);
            if(message != null)
                mTxtStatus.setText(message);
        }
        
        if ( Flag.SessionKeyExist.getState()== false)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"LoginRetry Flag.SessionKeyExist.getState():"+Flag.SessionKeyExist.getState());
	        mBtnSettings.setOnClickListener(new OnClickListener(){
	            @Override
	            public void onClick(View v)
	            {
	                Intent intent = new Intent();
                	intent.setClass(LoginRetryActivity.this, ProvisionSetupActivity.class);
                    startActivity(intent);
	                finish();
	            }
	        });
        }
        else
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Clickable:"+Clickable);
        	final Runnable RetryThread = new Runnable() {
                public void run() {
                	if(Flag.SessionKeyExist.getState() == true)
                	{
	            		if (Clickable == 0)
	            		{
	            			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"RetryThread ");
		                	Intent intent = new Intent();
			                intent.setClass(LoginRetryActivity.this, ProvisionLoginActivity.class);
			                startActivity(intent);
			                finish();
	            		}
                	}
                }
            };
            mHandler.postDelayed(RetryThread, 10000);
        	
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"LoginRetry Flag.SessionKeyExist.getState() :"+Flag.SessionKeyExist.getState());
        	mBtnSettings.setOnClickListener(new OnClickListener(){
	            @Override
	            public void onClick(View v)
	            {
	            	Clickable = 1;
	            	mBtnSettings.setClickable(false);
	                Intent intent = new Intent();
	                intent.setClass(LoginRetryActivity.this, ProvisionLoginActivity.class);
	                startActivity(intent);
	                finish();
	            }
	        });
        }
    }

    protected void onDestroy() 
    { 
        super.onDestroy();
        ViroActivityManager.setActivity(null);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG," onDestroy");
        mTxtStatus = null;
        mBtnSettings = null;
        message = null;
    /*    if (QTReceiver.engine(LoginRetryActivity.this,false) != null)
        	QTReceiver.engine(LoginRetryActivity.this,false).logout("");*/
    }
    
    @Override
    public void onPause()
    {
        super.onPause();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG," onPause:"+QTReceiver.engine(this, false).isLogon());
        if(QTReceiver.engine(this, false).isLogon())
            finish();
    }
    @Override
    public void onResume()
    {
        super.onResume();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onResume:"+QTReceiver.engine(this, false).isLogon());
        if(QTReceiver.engine(this, false).isLogon())
            finish();
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
    	//Log.d(TAG,"onKeyDown");
        if (keyCode == KeyEvent.KEYCODE_BACK ) 
        {
        	System.exit(0);
        }
        return super.onKeyDown(keyCode, event);
    }
}
