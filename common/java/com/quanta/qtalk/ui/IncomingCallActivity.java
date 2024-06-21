package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PtmsData;
import com.quanta.qtalk.util.QtalkUtility;


public class IncomingCallActivity extends AbstractIncomingCallActivity 
{
    private static final String DEBUGTAG = "QTFa_IncomingCallActivity";
    TextView callerID;
    TextView callerRole;
    ImageView facePhoto=null;
    Button videoBtn=null;
    Button voiceBtn=null;
    Button rejectBtn=null;
    View mView = null;
    WindowManager mWindowManager=null;
    String Name = null, Alias = null, PhoneNumber = null, Image = null, Role ="Unknown" ,Title = null, Role_UI = "";

    QtalkEngine engine;
    QtalkSettings qtalksetting ;
    
    private BroadcastReceiver session_event_receiver = new BroadcastReceiver(){

		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(DEBUGTAG, "IncomingCallActivity.onReceive(): "+intent.getAction());
			if(intent.getAction().equals(CommandService.ACCEPT_CALL)){
				accept(true, true); // accept in audio call
				Log.d(DEBUGTAG, "finish: 1");
				finish();
//				if(Role.contentEquals("doctor")){
//					accept(true,true);
//					finish();
//				}
//				else {
//					accept(true,true);
//					finish();
//				}
			}
			else if(intent.getAction().equals(CommandService.ACCEPT_VIDEOCALL)){
				accept(true, false); // accept in video call
				Log.d(DEBUGTAG, "finish: 2");
				finish();
			}
			else if(intent.getAction().equals(CommandService.HANGUP_CALL)){
				if(rejectBtn != null)
					rejectBtn.performClick();
//				hangup();
//				finish();
			} 
            else if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
//				if(rejectBtn != null)
//					rejectBtn.performClick();
//				hangup();
//				finish();
                Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
			}
		}
    };
    
	@Override
    protected void onCallMissed(String remoteID) 
    {
        //Add missed call notification
        String text = callerID.getText()+ getString(R.string.miss_call_notification);
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        
        PendingIntent intent = PendingIntent.getActivity(this, 0, QTReceiver.createCallLogIntent(), PendingIntent.FLAG_IMMUTABLE);
        Notification.Builder builder = new Notification.Builder(this);
        builder.setWhen(System.currentTimeMillis());
        builder.setSmallIcon(R.drawable.status_missed2);
        builder.setTicker(text);
        builder.setContentIntent(intent);
        
        Notification notification = builder.build();
        notification.flags |= Notification.FLAG_SHOW_LIGHTS;
        notification.ledARGB = 0xff0000ff; /* blue */
        notification.ledOnMS = 125;
        notification.ledOffMS = 2875;
        notification.flags |= Notification.FLAG_AUTO_CANCEL;


        notificationManager.notify(QTReceiver.NOTIFICATION_ID_CALLLOG, notification);
        Log.d(DEBUGTAG, "finish: 3");
        finish();
        
    }
    @SuppressLint("InflateParams")
	@Override
    public void onCreate(Bundle savedInstanceState)
    {
    	Log.d(DEBUGTAG,"==>onCreate" + this.hashCode());

        qtalksetting = QtalkUtility.getSettings(this, engine);

    	//------------------------------------QF3 HAND SET
    	AppStatus.setStatus(AppStatus.INCOMING);
		IntentFilter intentfilter = new IntentFilter();
		intentfilter.addAction(CommandService.ACCEPT_CALL);
		intentfilter.addAction(CommandService.ACCEPT_VIDEOCALL);
		intentfilter.addAction(CommandService.HANGUP_CALL);
		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(session_event_receiver, intentfilter);
    	//------------------------------------QF3 HAND SET
        
    	//------------------------------------QF3 LED        
		Intent intent = new Intent();
        intent.setAction("android.intent.action.vc.led");
        Bundle bundle = new Bundle();
        bundle.putString("KEY_LED_VALUE","2");
        intent.putExtras(bundle);
        sendBroadcast(intent);
        Log.d(DEBUGTAG, "send led event to launcher:2");
        //------------------------------------QF3 LED
        
        super.onCreate(savedInstanceState);
        
        /*****************Set last_activity**********************/
        ViroActivityManager.setActivity(this);
        Flag.InsessionFlag.setState(true);
        if(Hack.isPhone()) {
        	setContentView(R.layout.layout_incomingcall);
        	callerID=(TextView)findViewById(R.id.caller_name);
        	callerRole=(TextView)findViewById(R.id.roleName);
        }
        else {
        	
	        setContentView(R.layout.layout_broadcast);
	
		    WindowManager.LayoutParams params = new WindowManager.LayoutParams(
		            WindowManager.LayoutParams.MATCH_PARENT,
		            WindowManager.LayoutParams.MATCH_PARENT,
		            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
		            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
		                    | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
		                    | WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH
                            | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
		            PixelFormat.TRANSLUCENT);
	
		    mWindowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
		    LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
		    mView = inflater.inflate(R.layout.layout_incomingcall, null);
	
		    // Add layout to window manager
		    mWindowManager.addView(mView, params);
	        callerID=(TextView)mView.findViewById(R.id.caller_name);
	        callerRole=(TextView)mView.findViewById(R.id.roleName);		    
        }


        Log.d(DEBUGTAG,"callerID:"+ callerID+" callerRole:"+callerRole);
        String callerIDContent = mRemoteID;


        if (mContactQT!=null)
        	callerIDContent=mContactQT.getDisplayName();

        if(callerIDContent==null){
        	callerIDContent="Unknown caller";
        }
        
        Log.d(DEBUGTAG, "IncomingCallActivity.onCreate(), mRemoteID:" + callerIDContent);


        QtalkDB qtalkdb = new QtalkDB();
        PtmsData data = null;
        if(Hack.isPhone() || Hack.isStation()) {
            data = PtmsData.findData(callerIDContent);
        }
        if(data==null) {
            QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);
            
            Cursor cs = qtalkdb.returnAllPBEntry();
            int size = cs.getCount();
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"size="+size);
            cs.moveToFirst();
            int cscnt = 0;
            for( cscnt = 0; cscnt < cs.getCount(); cscnt++)
            {
                PhoneNumber = QtalkDB.getString(cs, "number");
                if (PhoneNumber.contentEquals(callerIDContent))
                {
                    Name = QtalkDB.getString(cs, "name");
                    Role = QtalkDB.getString(cs, "role");
                    Alias = QtalkDB.getString(cs, "alias");
                    Title = QtalkDB.getString(cs, "title");
                }
                cs.moveToNext();
            }
            cs.close();
            qtalkdb.close();
        }
        else {
            PhoneNumber = callerIDContent;
            Name = data.name;
            Role = "patient";
            Alias = data.alias;
            Title = data.title;
        }
        Image = Image+".jpg";
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Name:"+ Name+" Title:"+Title);

        String name_phrase = null;
        int max_str_len = 4;
        if(Role!=null && (!Role.contentEquals("patient"))){ //remove role from UI for 1.0.467
            max_str_len = 8;
        }
        if(qtalksetting.provisionType.contentEquals("?type=ns")
                && Role!=null
                && Role.contentEquals("patient")
                && (Alias != null) && (Alias.trim().length()>=3)) {
            Name = Alias;
        }
        if(Name == null)
            name_phrase =  callerIDContent;
        else if (Name.length() > max_str_len)
			name_phrase = Name.substring(0, max_str_len) + "...";
		else
			name_phrase = Name;
        if(Title!=null){
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"name_phrase:"+ name_phrase);
        	if(!Title.contentEquals("")){
        		if(Title.contentEquals("head_nurse")){
        			Role_UI = getString(R.string.head_nurse);
        		}else if(Title.contentEquals("clerk")) {
        			Role_UI = getString(R.string.secreatary);
        		}else{
            		Role_UI = Title;
        		}
        	}
        	else{
        		if(Role.contentEquals("nurse")){
        			Role_UI = getString(R.string.nurse);
        		}else if(Role.contentEquals("station")){
        			Role_UI = getString(R.string.nurse_station);
        		}else if(Role.contentEquals("staff")){
        			Role_UI = getString(R.string.cleaner);
        		}else{
        			Role_UI = "";
        		}
        	}
        	callerID.setText(name_phrase);
            callerRole.setText(Role_UI);
        }
        else{
	        if(Name!=null){
	        	if(Role.contentEquals("nurse")){
	        		Role_UI = getString(R.string.nurse);
	        	}else if(Role.contentEquals("station")){
	        		Role_UI = getString(R.string.nurse_station);
	        	}else if(Role.contentEquals("staff")){
	        		Role_UI = getString(R.string.cleaner);
	        	}else{
	        		Role_UI = "";
	        	}
        	
	            callerID.setText(name_phrase);
	            callerRole.setText(Role_UI);
	            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "Role_UI="+Role_UI+" Name="+name_phrase);
	        }else{
	        	Role_UI = "";
	        	callerRole.setVisibility(View.GONE);
	            callerID.setText(callerIDContent);
	        }
        }

        if(Role!=null){
	        if(!Role.contentEquals("patient")){ //remove role from UI for 1.0.467
	            callerRole.setVisibility(View.GONE);
	        }
        }
        if(Hack.isPhone())
        	facePhoto=(ImageView)findViewById(R.id.faceiconPic);
        else
        	facePhoto=(ImageView)mView.findViewById(R.id.faceiconPic);

        if(Role == null){
        	Role = "unknown";
        }
        if(Role.contentEquals("nurse")){
        	if(Role_UI.contentEquals(getString(R.string.secreatary))){
    			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_secreatary));
    		}else{
    			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_nurse));
    		}
		}
		else if(Role.contentEquals("station")){
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_nurse_station));
		}
		else if(Role.contentEquals("mcu")){
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_broadcast));
		}
		else if(Role.contentEquals("family")){
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
		}
		else if(Role.contentEquals("patient")){
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
		}else if(Role.contentEquals("staff")){
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_cleaner));
		}
		else{
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
		}

        if(Hack.isPhone()) {
        	voiceBtn=(Button)findViewById(R.id.voiceBtn);
        	videoBtn=(Button)findViewById(R.id.videoBtn);
        	rejectBtn = (Button)findViewById(R.id.rejectBtn);
        }
        else {
        	voiceBtn=(Button)mView.findViewById(R.id.voiceBtn);
        	videoBtn=(Button)mView.findViewById(R.id.videoBtn);
        	rejectBtn = (Button)mView.findViewById(R.id.rejectBtn);
        }

        /* linzl: while the "if"?
		if(super.mEnableVideo || Role.contentEquals("doctor") 
				|| !(callerIDContent.startsWith(QtalkSettings.qsptNumPrefix)) ){
            videoBtn=(Button)mView.findViewById(R.id.videoBtn);
        }
		else{
            videoBtn=(Button)mView.findViewById(R.id.videoBtn);
        }
		*/
        
        OnClickListener theclick = new OnClickListener(){
            @Override
            public void onClick(View v){
                int selectN=v.getId();
                if(selectN==R.id.videoBtn){
                    accept(true,false);
                    Log.d(DEBUGTAG, "finish: 4");
                    finish();
                }
                else if(selectN==R.id.voiceBtn){
                    accept(true,true);
                    Log.d(DEBUGTAG, "finish: 5");
                    finish();
                }
                else if(selectN==R.id.rejectBtn){
                    hangup();
                    Log.d(DEBUGTAG, "finish: 6");
                    finish();
                }   
            }
        };
        
        Log.d(DEBUGTAG,"super.mEnableVideo "+ mEnableVideo);
        if(videoBtn!=null && mEnableVideo){
            videoBtn.setOnClickListener(theclick);
        }
        if(voiceBtn!=null)
        	voiceBtn.setOnClickListener(theclick);	
        
        if(rejectBtn!=null)
        	rejectBtn.setOnClickListener(theclick);
            
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED  
            | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD  
            | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON  
            | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);

        if(savedInstanceState!=null){
            if(!savedInstanceState.containsKey("KEY_PERFORM_ACCEPT")) {
                Log.e(DEBUGTAG,"KEY_PERFORM_ACCEPT in savedInstanceState is not exist!");
            }
            else if(savedInstanceState.getBoolean("KEY_PERFORM_ACCEPT")){
                voiceBtn.performClick();
            }
        }
        else{
            Bundle bundle1 = this.getIntent().getExtras();
            if(bundle1 == null) { //prevent bundle1 == null
                Log.e(DEBUGTAG,"bundle1 == null");
            }
            else if(!bundle1.containsKey("KEY_PERFORM_ACCEPT")) {
                Log.e(DEBUGTAG,"KEY_PERFORM_ACCEPT in bundle1 is not exist!");
            }
            else if(bundle1.getBoolean("KEY_PERFORM_ACCEPT")){
                Log.d(DEBUGTAG,"Auto Answer becuz MCU");
                voiceBtn.performClick();
            }
        }
		if (Hack.isPhone()) {
			NotificationManager notificationManager = (NotificationManager) getApplicationContext()
					.getSystemService(Context.NOTIFICATION_SERVICE);
			notificationManager.cancel(1);
		}
        if(Hack.isQF3a())
            Hack.dumpAllResource(this, DEBUGTAG);
   }//end of onCreate
    
    @Override
    protected void onDestroy()
    {
		Log.d(DEBUGTAG, "==>onDestroy" + this.hashCode());
		super.onDestroy();
		if(!Hack.isPhone()) {
			mView.setVisibility(View.INVISIBLE);
			try {
				mWindowManager.removeViewImmediate(mView);
			}
			catch(Exception e) {
			    Log.e(DEBUGTAG,"remove view from window manager is error", e);
			}
		}
		releaseAll();
		Log.d(DEBUGTAG, "<==onDestroy" + this.hashCode());
    }
    
    @Override
    public void onSelfCallChange(boolean enableVideo)
    {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void onRemoteCallChange(boolean enableVideo)
    {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void onReinviteFailed()
    {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void onRemoteHangup(final String msg)
    {
    	RingingUtil.playSoundHangup(this);
    	AppStatus.setStatus(AppStatus.IDEL);
    	Log.d(DEBUGTAG, "finish: 7");
    	finish();
    }
    @Override
    public void onRemoteCallHold(boolean isHeld)
    {
		// not used
        
    }
    @Override
    public void onRequestIDR()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onRequestIDR");
    }
    
    private void releaseAll()
    {
    	unregisterReceiver(session_event_receiver);
        callerID=null;
        callerRole=null;
        facePhoto.getDrawable().setCallback(null);
        facePhoto=null;
        videoBtn=null;
        voiceBtn=null;
        rejectBtn=null;
    }
}
