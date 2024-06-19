package com.quanta.qtalk.ui;

import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.format.Time;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.FileUtility;
import com.quanta.qtalk.provision.PackageChecker;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.history.HistoryDataBaseUtility;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;

import javax.xml.parsers.ParserConfigurationException;

public class QtalkCheckActivity2 extends AbstractSetupActivity{
//	private final String TAG = "QtalkCheckActivity2";
	private static final String DEBUGTAG = "QTFa_QtalkCheckActivity2";
	private String provision_uri = null;
	private String phonebook_uri = null;
/*	private String version_uri = null;
	private String versionNumber = null;*/
	private String session = "000000000000";
	private final int PHONEBOOK_UPDATE_SUCCESS = 2;
	private final int PROVISION_UPDATE_SUCCESS = 1;
	private final int LOGIN_PROVISION_SUCCESS = 3;
	private final int LOGIN_PHONEBOOK_SUCCESS = 4;
//	private final int VERSION_CHECK_SUCCESS = 5;
	private final int DEACTIVATION_SUCCESS = 6;
	private final int FAIL = 0;
	public int provision_time   = 0;
	public int phonebook_time   = 0;

	QtalkSettings qtalk_settings;
	HistoryDataBaseUtility historydatabase =null;
	private ProvisionInfo provision_info = null;
	private TextView versionName;
	private LinearLayout mFullLoginSetup = null;
	
	private Bundle msgData = null;
	
	//TODO:
	//1. fetch provision file (provision file, phonebook)
	//2. check 
	
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(DEBUGTAG,"==>onCreate "+this.hashCode());
        setContentView(R.layout.layout_provision_login);
        mFullLoginSetup = (LinearLayout) findViewById(R.id.FullLayoutSetup);
    	if(QtalkSettings.Login_by_launcher==true){
    		mFullLoginSetup.setVisibility(View.INVISIBLE);
    		//mFullLoginSetup.setBackgroundColor(Color.GREEN);
    	}
        if (this.getIntent().getExtras() != null)
        {
	        session = this.getIntent().getExtras().getString("session");
        }
        try {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "Reprovision Flag is "+Flag.Reprovision.getState());
			qtalk_settings = super.getSetting(false);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"onCreate",e);
		}
       // if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
        //{
        	File dirFile = new File(Hack.getStoragePath());
        	Log.d(DEBUGTAG,"dirFile.exists(): "+dirFile.exists());
        	if (!dirFile.exists())
        	{
        		dirFile.mkdir();
        	}
        //}
     
        TextView mUserName = (TextView) findViewById(R.id.mUserName);
//        TextView mCSServer = (TextView) findViewById(R.id.mCSServer);
        TextView mPassWord = (TextView) findViewById(R.id.mPassWord);
        mUserName.setText(qtalk_settings.provisionID);
//        mCSServer.setText(qtalk_settings.provisionServer);
        mPassWord.setText(qtalk_settings.provisionPWD);
        versionName = (TextView) findViewById(R.id.versionName);
    	String version=null;
		int versionCode=0;
		try {
			version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
			versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
			versionName.setText("QSPT-VC "+version);
			//versionName.setText("QSPT-VC "+version+" ("+versionCode+") ");
			
		} catch (NameNotFoundException e1) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"getVersion",e1);
			Log.e(DEBUGTAG,"",e1);
		}
		Log.d(DEBUGTAG,"<==onCreate "+this.hashCode());
    }
    
    
/*    public void update(File file)
    {   
    	Intent intent = new Intent();
    	intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    	intent.setAction(android.content.Intent.ACTION_VIEW);
    	String type = "application/vnd.android.package-archive";
    	intent.setDataAndType(Uri.fromFile(file), type);
    	startActivity(intent);
    }*/
    
    
    Handler mHandler = new Handler(){
    	Time t = new Time();
		public void handleMessage(final Message msg)
        {
			Log.d(DEBUGTAG,"msg.what:"+msg.what);
    		switch(msg.what)
            {
            	default: break;
            	case PHONEBOOK_UPDATE_SUCCESS:
	            //	t.setToNow();
    				//qtalk_settings.lastPhonebookFileTime = t.format("%Y%m%d%H%M%S")+"000";
            		qtalk_settings.lastPhonebookFileTime = msg.getData().getString("timestamp");
            		QThProvisionUtility.provision_info.lastPhonebookFileTime = msg.getData().getString("timestamp");
            		provision_time = Integer.valueOf(QThProvisionUtility.provision_info.general_period_check_timer)*1000;
            		phonebook_time = Integer.valueOf(QThProvisionUtility.provision_info.short_period_check_timer)*1000;
					QtalkDB qtalkdb = new QtalkDB();
            		
            		qtalkdb.insertPTEntry(qtalk_settings.lastPhonebookFileTime,qtalk_settings.lastProvisionFileTime);
					Cursor cs = qtalkdb.returnAllATEntry();
			        cs.close();
			        qtalkdb.close();
			        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "qtnMessenger.provision_info.lastProvisionFileTime:"+QThProvisionUtility.provision_info.lastProvisionFileTime);
    				try {
						QtalkCheckActivity2.super.updateSetting(QThProvisionUtility.provision_info);
					} catch (FailedOperateException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG, "PHONEBOOK_UPDATE_SUCCESS context=null");
						Message msg1 = new Message();
			            msg1.what = FAIL;
			            Bundle bundle = new Bundle();
			            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
			            msg1.setData(bundle);
			            mHandler.sendMessage(msg1);
					}
					
					QtalkSettings qtalk_settings2 = null;
					try {
						qtalk_settings2 = QtalkCheckActivity2.super.getSetting(false);
					} catch (FailedOperateException e1) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"PHONEBOOK_UPDATE_SUCCESS",e1);
						Log.e(DEBUGTAG,"",e1);
					};
					Flag.SIPRunnable.setState(true);
                	Intent myintent = new Intent();
                	if(QTReceiver.engine(QtalkCheckActivity2.this, false).isLogon())
                    {
                    	myintent.setClass(QtalkCheckActivity2.this,com.quanta.qtalk.ui.ContactsActivity.class);
                 	}
                    else
                    {
                    	myintent.setClass(QtalkCheckActivity2.this,com.quanta.qtalk.ui.SIPLoginActivity.class);
                    }
                    QtalkCheckActivity2.this.startActivity(myintent);
                    
        			//------------------------------------------------------------[PT]
                    Intent broadcast_intent = new Intent();
                    broadcast_intent.setAction("android.intent.action.vc.phonebookupdate");
    		        Log.d("SEND","Send Broadcast to phonebookupdate SUCCESS");
    		        sendBroadcast(broadcast_intent);
    		        //------------------------------------------------------------[PT]
                    
                    finish();
            		break;
    			case PROVISION_UPDATE_SUCCESS:
    				msgData = msg.getData();
    				new Thread(provision_update_sccess).start();
    				break;
    			case LOGIN_PROVISION_SUCCESS:
					t.setToNow();
    				phonebook_uri = QThProvisionUtility.provision_info.phonebook_uri+
                    //String phonebook_uri = PackageChecker.getProvisionInfo().phonebook_uri +   "/~kakasi/provision/provision.php"+
                    	qtalk_settings.provisionType+
    					"&service=phonebook&action=check&uid="+
                    	qtalk_settings.provisionID+"&session="+qtalk_settings.session+"&timestamp="+qtalk_settings.lastPhonebookFileTime;
    				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"phonebook_uri:"+ phonebook_uri);
                    
    		    	new Thread( new Runnable(){

    		            @Override
    		            public void run()
    		            { 
		                    try {
									qtnMessenger.httpsConnect(phonebook_uri,session,qtalk_settings.lastPhonebookFileTime,qtalk_settings.provisionID,qtalk_settings.provisionType,qtalk_settings.provisionServer,qtalk_settings.key);
		                    } catch (IOException e1) {
								// TODO Auto-generated catch block
		                    	Log.e(DEBUGTAG,"LOGIN_PROVISION_SUCCESS",e1);
								Log.e(DEBUGTAG,"",e1);
								Message msg1 = new Message();
					            msg1.what = FAIL;
					            Bundle bundle = new Bundle();
					            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
					            msg1.setData(bundle);
					            mHandler.sendMessage(msg1);
							}
                    //qtalkdb.close();
    		        }}).start();
    				break;
    			case LOGIN_PHONEBOOK_SUCCESS:
    				//qtalk_settings.lastProvisionFileTime = t.format("%Y%m%d%H%M%S")+"000";                  
                    // "/pod/dispatch.do"
                    Flag.SIPRunnable.setState(true);
                    myintent = new Intent();
                    if(QTReceiver.engine(QtalkCheckActivity2.this, false).isLogon())
                    {
                    	myintent.setClass(QtalkCheckActivity2.this,com.quanta.qtalk.ui.ContactsActivity.class);
                 	}
                    else
                    {
                    	myintent.setClass(QtalkCheckActivity2.this,com.quanta.qtalk.ui.SIPLoginActivity.class);
                    }
                    QtalkCheckActivity2.this.startActivity(myintent);
                    finish();
            		break;
    			case DEACTIVATION_SUCCESS:
    				
    				FileUtility.delFolder(Hack.getStoragePath());
					{
						qtalkdb = new QtalkDB();
						Log.d(DEBUGTAG, "qtalkdb.deactiveation()");
						qtalkdb.deactiveation();
						qtalkdb.close();
					}
    				Log.d(DEBUGTAG,"exitQT();");
    				exitQT();
			
    				if(QThProvisionUtility.provision_info != null){
    					QThProvisionUtility.provision_info.lastProvisionFileTime = "";
    					qtalk_settings.lastProvisionFileTime = "0";
    				
    					qtalk_settings.lastPhonebookFileTime = "0";
    					QThProvisionUtility.provision_info.lastPhonebookFileTime = "";
    					try {
    						QtalkCheckActivity2.super.updateSetting(QThProvisionUtility.provision_info);
    					} catch (FailedOperateException e) {
    						// TODO Auto-generated catch block
							Log.e(DEBUGTAG, "DEACTIVATION_SUCCESS context=null");
    					}
    		            PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);
    				}
    				if(historydatabase == null)
    				{
    					historydatabase = new HistoryDataBaseUtility(getApplicationContext());
    					historydatabase.open(true);
    				}
    				Log.d(DEBUGTAG,"historydatabase.deactivate()");
    				historydatabase.deactivate();
            		Log.d(DEBUGTAG,"release_test Flag.Activation= "+Flag.Activation.getState()+
            				"line="+Thread.currentThread().getStackTrace()[2].getLineNumber());
    				Flag.Activation.setState(false);
    				Flag.SessionKeyExist.setState(false);
    				Bundle bundle = new Bundle();
  		            bundle.putString(ProvisionSetupActivity.MESSAGE, "認證失敗, 已超過使用期限\r\n");
  		         //   Flag.Reprovision.setState(false);
  		          if(!(qtalk_settings.provisionType.equals("?type=hh"))){  // [PT] for PT case, when patient is discharged from
  		        	                                             //      hospital, the deactivate UI should not disappear
  		            myintent = new Intent();
                    myintent.putExtras(bundle);
                    myintent.setClass(QtalkCheckActivity2.this,com.quanta.qtalk.ui.ProvisionSetupActivity.class);
                    myintent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
                    QtalkCheckActivity2.this.startActivity(myintent);
  		          }
  		          	//-------------------------------------------------------[PT]

  		          	Intent broadcast_logout = new Intent();
  		          	broadcast_logout.setAction("android.intent.action.vc.logout.cb");
  		          	Log.d("SEND","Send Broadcast to vc logout SUCCESS");
  		          	sendBroadcast(broadcast_logout);
  		          	
  		            //-------------------------------------------------------[PT]
                    finish();
                    break;
    			case FAIL:
    				if(QThProvisionUtility.provision_info != null){ 
    					QThProvisionUtility.provision_info.lastProvisionFileTime = "";
    				
    				
	    				qtalk_settings.lastPhonebookFileTime = "0";
	    				QThProvisionUtility.provision_info.lastPhonebookFileTime = "";
	    				try {
							QtalkCheckActivity2.super.updateSetting(QThProvisionUtility.provision_info);
						} catch (FailedOperateException e) {
							// TODO Auto-generated catch block
							Log.e(DEBUGTAG, "FAIL context=null");
						}
	                    PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);
    				}
    				Intent intent = new Intent();
                    intent.putExtras(msg.getData());
                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"redirect_url:"+msg.getData().getString("redirect_url"));
                    intent.setClass( QtalkCheckActivity2.this, com.quanta.qtalk.ui.LoginRetryActivity.class);
                    QtalkCheckActivity2.this.startActivity(intent);
                    finish();
    				break;
            }
        }
    };
    QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);

    
	 Runnable provision_update_sccess = new Runnable(){  
		    @Override  
		    public void run() {  

				qtalk_settings.lastProvisionFileTime = msgData.getString("timestamp");
                qtalk_settings.session=msgData.getString("session");
                qtalk_settings.key=msgData.getString("key");
				if( QThProvisionUtility.provision_info == null)
				{
					Message msg1 = new Message();
		            msg1.what = FAIL;
		            Bundle bundle = new Bundle();
		            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
		            msg1.setData(bundle);
		            mHandler.sendMessage(msg1);
		            Thread.interrupted();
				}
				if(QThProvisionUtility.provision_info != null)
				{
					QThProvisionUtility.provision_info.lastProvisionFileTime = msgData.getString("timestamp");
					QThProvisionUtility.provision_info.session = msgData.getString("session");
					qtalk_settings.provisionServer = QThProvisionUtility.provision_info.provision_uri;
					QtalkDB qtalkdb = new QtalkDB();
				//	qtalkdb.DeleteATEntry("QTALKDB_AUTHENTICATION", "session");
			//		
					if (qtalk_settings.lastPhonebookFileTime != null && qtalk_settings.lastProvisionFileTime != null)
						qtalkdb.insertPTEntry(qtalk_settings.lastPhonebookFileTime,qtalk_settings.lastProvisionFileTime);
			//		qtalkdb.insertATEntry(qtalk_settings.session,qtalk_settings.key,qtnMessenger.provision_info.provision_uri,qtalk_settings.provisionID,versionNumber);
					Cursor cs1 = qtalkdb.returnAllATEntry();
					int size = cs1.getCount();
					if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"DB size="+size);
                	
                	if (size > 0)
                	{
                		cs1.moveToFirst();
                		Log.d(DEBUGTAG,"release_test Flag.Activation= "+Flag.Activation.getState()+
                				"line="+Thread.currentThread().getStackTrace()[2].getLineNumber());
                		Flag.Activation.setState(true);
				        int cscnt = 0;
				        for( cscnt = 0; cscnt < cs1.getCount(); cscnt++)
				        {
				        	qtalk_settings.session = QtalkDB.getString(cs1, "Session");
				        	qtalk_settings.key = QtalkDB.getString(cs1, "Key");
				  //      	qtalk_settings.provisionServer = cs1.getString(cs1.getColumnIndex("Server"));
				   //     	Log.d(TAG,"Server:"+ cs1.getString(cs1.getColumnIndex("Server")));
				        	cs1.moveToNext();
				        }
                	}
                	cs1.close();
					qtalkdb.close();
				}
				try {
					QtalkCheckActivity2.super.updateSetting(QThProvisionUtility.provision_info);
				} catch (FailedOperateException e) {
					// TODO Auto-generated catch block
					Log.e(DEBUGTAG, "PROVISION_UPDATE_SUCCESS context=null");
				}
                PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "SIPID:"+QThProvisionUtility.provision_info.authentication_id+"       SIPPW:"+QThProvisionUtility.provision_info.authentication_password);

				phonebook_uri = QThProvisionUtility.provision_info.phonebook_uri+
                //String phonebook_uri = PackageChecker.getProvisionInfo().phonebook_uri +   "/~kakasi/provision/provision.php"+
                	qtalk_settings.provisionType+
					"&service=phonebook&action=check&uid="+
                	qtalk_settings.provisionID+"&session="+qtalk_settings.session+"&timestamp="+qtalk_settings.lastPhonebookFileTime;
				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"phonebook_uri:"+ phonebook_uri);
                
                try {
       //         	if( phonebook_uri.startsWith("https"))
                	{
						qtnMessenger.httpsConnect(phonebook_uri,session,qtalk_settings.lastPhonebookFileTime,qtalk_settings.provisionID,qtalk_settings.provisionType,QThProvisionUtility.provision_info.phonebook_uri,qtalk_settings.key);
	            	}

                } catch (IOException e1) {
					// TODO Auto-generated catch block
                	Log.e(DEBUGTAG,"PROVISION_UPDATE_SUCCESS",e1);
					Log.e(DEBUGTAG,"",e1);
					Message msg1 = new Message();
		            msg1.what = FAIL;
		            Bundle bundle = new Bundle();
		            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
		            msg1.setData(bundle);
		            mHandler.sendMessage(msg1);
				}		    
            }  
		} ;
    
    
    @Override
    public void onStart()
    {
    	Log.d(DEBUGTAG,"==>onStart "+this.hashCode());
    	super.onStart();
    	Flag.SIPRunnable.setState(false);
		Log.d(DEBUGTAG,"release_test Flag.Activation= "+Flag.Activation.getState()+
				"line="+Thread.currentThread().getStackTrace()[2].getLineNumber());
    	Flag.Activation.setState(true);
    	historydatabase = new HistoryDataBaseUtility(getApplicationContext());
    	QtalkDB qtalkdb = new QtalkDB();
    	Cursor cs = qtalkdb.returnAllATEntry();
    	cs.moveToFirst();
        int cscnt = 0;
        for( cscnt = 0; cscnt < cs.getCount(); cscnt++)
        {
			qtalk_settings.session = QtalkDB.getString(cs, "Session");
			qtalk_settings.key = QtalkDB.getString(cs, "Key");
   //     	qtalk_settings.provisionServer = cs.getString(cs.getColumnIndex("Server"));
    //    	Log.d(TAG,"Server:"+ cs.getString(cs.getColumnIndex("Server")));
        	cs.moveToNext();
        }
        cs.close();
        Cursor cs1 = qtalkdb.returnAllPTEntry();
    	cs1.moveToFirst();
        int cscnt1 = 0;
        for( cscnt1 = 0; cscnt1 < cs1.getCount(); cscnt1++)
        {
			qtalk_settings.lastProvisionFileTime = QtalkDB.getString(cs1, "lastProvisionFileTime");
			qtalk_settings.lastPhonebookFileTime = QtalkDB.getString(cs1, "lastPhonebookFileTime");
			cs1.moveToNext();
        }
        cs1.close();
        qtalkdb.close();
        
        
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
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"parseProvisionConfig",e);
			};
			qtalk_settings.provisionServer = provision_info.provision_uri;
		}
		
		try {
				//qtalk_settings = super.getSetting(false);
				qtalk_settings.session = session;
				super.updateSetting(qtalk_settings, false);
		    	new Thread( new Runnable(){

	            @Override
	            public void run()
	            {           	
	            	try {
			            	Time t = new Time();
			            	t.setToNow();	     
			            	// time format = YYYYMMDDHHmmSS000
			            	//provision_uri = "https://"+qtalk_settings.provisionServer+
			            		//"/pod/dispatch.do?model=qtha&service=provision&action=check&session="+
			            		//session+"&timestamp="+t.format("%Y%m%d%H%M%S")+"000";   "/pod/dispatch.do" "/~kakasi/provision/provision.php"+
			            	provision_uri = qtalk_settings.provisionServer+
			            	qtalk_settings.provisionType+
			            	"&service=provision&action=check&uid="+
			            	qtalk_settings.provisionID+"&session="+qtalk_settings.session+"&timestamp="+qtalk_settings.lastProvisionFileTime;
			            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"provision url:"+provision_uri);
		            		qtnMessenger.httpsConnect(provision_uri,session,qtalk_settings.lastProvisionFileTime,qtalk_settings.provisionID,qtalk_settings.provisionType,qtalk_settings.provisionServer,qtalk_settings.key);
	            	} catch (IOException e) 
	            	{
						// TODO Auto-generated catch block
	            		Log.e(DEBUGTAG,"onStart",e);
						Message msg = new Message();
			            msg.what = FAIL;
			            Bundle bundle = new Bundle();
			            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路無法連線\r\n");
			            msg.setData(bundle);
			            mHandler.sendMessage(msg);
					}   	
	            	
	    	}}).start();
		} catch (FailedOperateException e1) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"onStart",e1);
			Log.e(DEBUGTAG,"",e1);
		}
		Log.d(DEBUGTAG,"<==onStart "+this.hashCode());
    }
    @Override
    	protected void onDestroy() {
    		Log.d(DEBUGTAG,"==>onDestroy "+this.hashCode());
    		// TODO Auto-generated method stub
    		super.onDestroy();
    		if(mFullLoginSetup!=null)
    			mFullLoginSetup.removeAllViews();
    		Log.d(DEBUGTAG,"<==onDestroy "+this.hashCode());
    	}
}
