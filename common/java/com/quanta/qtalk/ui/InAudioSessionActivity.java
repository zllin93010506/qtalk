package com.quanta.qtalk.ui;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

import java.util.ArrayList;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

public class InAudioSessionActivity extends AbstractInAudioSessionActivity {

    private TextView mTimerTextView = null;
    private TextView mTimerConnectTextView = null;
    ImageView facePhoto=null;
    Button mEndBtn=null; 
    Button mSwitchBtn=null;
    RelativeLayout mSwitch_ll = null;
    Button reject_btn = null;
    Button accept_btn = null;
//    private static final String TAG = "InAudioSessionActivity";
    private static final String DEBUGTAG = "QTFa_InAudioSessionActivity";
    //for dtmf
  //  private static final String keyValue[]={"1","2","3","4","5","6","7","8","9","*","0","#","+"};
    TextView inputNum=null;
    OnClickListener mTheClick = null;
    private String Name = null, PhoneNumber = null, Role = null, Title = null, Role_UI = null;
    private String SIPNumber = null;
//    private String Image = null;    
    
    public static final int     CONFIGURE_MENU_ITEM=Menu.FIRST;
    Timer timer;
    TimerTask task;
    Timer hTimer;
    TimerTask hTask;
    public ArrayList<Map<String, Object>> mData = new ArrayList<Map<String, Object>>();
    
    QtalkEngine   engine;
    QtalkSettings qtalksetting ;
    
    AlertDialog switch_call_waiting = null;
    AlertDialog alert_dialog =null;
    
    ToggleButton speaker;
    protected final Handler swHandler = new Handler();
    final Runnable WaitingTimeOut = new Runnable() {
        public void run() {
        	if(switch_call_waiting!=null)
        		switch_call_waiting.cancel();
        	}
        };
    final Runnable RequestTimeOut = new Runnable() {
        public void run() {
        		if(reject_btn!=null)
           			reject_btn.performClick();
           	}
        };     
    private BroadcastReceiver session_event_receiver = new BroadcastReceiver(){

		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(DEBUGTAG,"onReceive:"+intent.getAction());
			if(intent.getAction().equals(CommandService.HANGUP_CALL)){
				hangup(false);
				AppStatus.setStatus(AppStatus.IDEL);
				finish();        
			}
            else if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
                //hangup(false);
				//AppStatus.setStatus(AppStatus.IDEL);
				//finish();
                Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
			}
		}
    };

//    BroadcastReceiver wifi_connect_action_br;

 /*   private void DisableAllCounter()
    {
		if( hTimer != null && hTask != null)
		{
    		hTimer.cancel();
    		hTask.cancel();
			hTimer.purge();
		}
		
		if( timer != null && task != null)
		{
			timer.cancel();
			task.cancel();
    		timer.purge();
		}
    }*/
	protected void onPause() {
		Log.d(DEBUGTAG, "==>onPause");
		hangup(false);
		finish();
		super.onPause();
	};
    @Override
    protected void onDestroy()
    {
		Intent intent = new Intent(this, QTReceiver.class);
		intent.setAction(QTService.ACTION_PHONEBOOK);
		intent.putExtra("pause", false);
		sendBroadcast(intent);
        super.onDestroy();
    	releaseAll();
    }
    //
    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
    	//------------------------------------QF3 HAND SET
    	AppStatus.setStatus(AppStatus.SESSION);
		IntentFilter intentfilter = new IntentFilter();
		intentfilter.addAction(CommandService.ACCEPT_CALL);
		intentfilter.addAction(CommandService.HANGUP_CALL);
		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(session_event_receiver, intentfilter); 
    	
    	//------------------------------------QF3 HAND SET
        qtalksetting = QtalkUtility.getSettings(this, engine);
        
        ViroActivityManager.setActivity(this);
        setContentView(R.layout.layout_inaudiosession_pt);
        //////////////
        //dm = new DisplayMetrics();
        //getWindowManager().getDefaultDisplay().getMetrics(dm);
        speaker = (ToggleButton)findViewById(R.id.btn_speakerstatus);
        speaker.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if(speaker.isChecked()){
					enableLoudSpeaker(true);
				}else{
					enableLoudSpeaker(false);
				}
			}
		});
        if(Hack.isTabletDevice(getApplicationContext())){
        	speaker.setVisibility(View.INVISIBLE);
        }
        
        //Caller ID======================================
        TextView callerID=(TextView)findViewById(R.id.callerName);//callerName
        TextView callerRole=(TextView)findViewById(R.id.roleName);

        String callerIDContent = mRemoteID;
        SIPNumber = mRemoteID;
        if (mContactQT!=null)
            callerIDContent=mContactQT.getDisplayName();
                //Toast.makeText(IncomingCallActivity.this,"caller="+callerIDContent,Toast.LENGTH_SHORT).show();                
        if(callerIDContent==null){
                    //Log.d(DEBUGTAG,"callerIDContent="+callerIDContent);
            callerIDContent="Unknown caller";
                    //TODO
        }        

        QtalkDB qtalkdb = QtalkDB.getInstance(this);
    	
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
				Title = QtalkDB.getString(cs, "title");
			}
        	cs.moveToNext();
        }
        String name_phrase = null;
        if(Title!=null){
            if(Name == null){
                name_phrase = callerIDContent;
            }else if (Name.length() > 9){
                name_phrase = Name.substring(0, 9) + "...";
            }else{
                name_phrase = Name;
            }
        	if(!Title.contentEquals("")){
        		if(Title.contentEquals("head_nurse")){
        			Role_UI = getString(R.string.head_nurse);
        		}else if(Title.contentEquals("clerk")) {
        			Role_UI = getString(R.string.secreatary);
        		}else{
            		Role_UI = Title;
        		}
        	}else{
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
        }else{
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
	            if (Name.length() > 9){
	                name_phrase = Name.substring(0, 9) + "...";
	            }else{
	                name_phrase = Name;
	            }
	            callerID.setText(name_phrase);
	            callerRole.setText(Role_UI);
	        }else{
	        	Role_UI = "";
	        	callerRole.setVisibility(View.GONE);
	            callerID.setText(callerIDContent);
	        }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "Role_UI="+Role_UI+" Name="+name_phrase);
        if(Role!=null){
	        if(!Role.contentEquals("patient")){ //remove role from UI for 1.0.467
	        	callerRole.setVisibility(View.GONE);
	        }
        }else{
        	callerRole.setVisibility(View.GONE);
        }
 //       Image = Image+".jpg";
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Name:"+ Name);
        //callerID.setText(Name);
        //callerRole.setText(Role_UI);
        cs.close();
        qtalkdb.close();
        
        
        facePhoto=(ImageView)findViewById(R.id.faceiconPic);
        if(Role == null){
        	Role = "unknown";
        }
        if(Role.contentEquals("nurse")){
        		if(Role_UI.contentEquals(getString(R.string.secreatary))){
        			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_secretary));
        		}else{
        			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_nurse));
        		}
			}
			else if(Role.contentEquals("station")){
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_nurse_station));
			}
			else if(Role.contentEquals("mcu")){
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_broadcast));
			}
			else if(Role.contentEquals("family")){
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_patient));
			}
			else if(Role.contentEquals("patient")){
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_patient));
			}
			else if(Role.contentEquals("staff")){
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_cleaner));
			}
			else{
				facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_incall_patient));
			}

              //  callerID.setText(callerIDContent);            

        //CONTACT PHOTO==================================================
        /*facePhoto=(ImageView)findViewById(R.id.faceiconPic);
        File file_temp = new File (Image);
		Log.d(TAG, "file="+file_temp);
        if (file_temp.exists())
		{	
        	Drawable Temp1 = DrawableTransfer.LoadImageFromWebOperations(Image);
            facePhoto.setImageDrawable(Temp1);
		}
		else
		{
			facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.all_icn_member));
		}*/
             /*   if(mContactQT!=null){
                    Bitmap myPhoto=mContactQT.getBitmap(this);
                    if(myPhoto!= null){
                        facePhoto.setImageBitmap(myPhoto);
                    }else{
                        //Log.d(DEBUGTAG,"Null myPhoto="+myPhoto);                
                    }            
                }*/
        //Timer================================================ 
        mTimerTextView = (TextView)findViewById(R.id.voicetimer);
        mTimerConnectTextView = (TextView)findViewById(R.id.video_connection);
        
        if(getString(R.string.language).contentEquals("English")){
        	mTimerConnectTextView.setVisibility(View.GONE);
        }

        //Bottom BUTTONS======================================================================
        //int tmpAlpha=40;
        //BUTTONS=====================================
        mEndBtn=(Button)findViewById(R.id.endBtn); 
        mSwitchBtn=(Button)findViewById(R.id.btn_videoChange);
        mSwitch_ll = (RelativeLayout)findViewById(R.id.rl_videoChange);
        OnClickListener theclick=new OnClickListener(){
            @Override
            public void onClick(View v){
                int selectN=v.getId();
				//check if InAudioSessionActivity is top Activity.
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"selectN:"+selectN);
                if(ViroActivityManager.isCurrentPageTopActivity(InAudioSessionActivity.this,InAudioSessionActivity.class.getCanonicalName()))
                {
                    if(selectN==R.id.endBtn)
                    {
                        hangup(false);
                        AppStatus.setStatus(AppStatus.IDEL);
                        finish();
                    }
                }
            }
        };
        
        if (mEndBtn != null)
            mEndBtn.setOnClickListener(theclick);
        
        OnClickListener Switchclick=new OnClickListener(){

			@Override
			public void onClick(View v) {
				int selectN=v.getId();
				//check if InAudioSessionActivity is top Activity.
				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"selectN:"+selectN);
				if (ViroActivityManager.isCurrentPageTopActivity(
						InAudioSessionActivity.this,
						InAudioSessionActivity.class.getCanonicalName())) {
					AlertDialog.Builder builder = new AlertDialog.Builder(
							InAudioSessionActivity.this);

					if (switch_call_waiting == null) {
						switch_call_waiting = builder
								.setTitle(R.string.app_name)
								.setMessage("waiting").setCancelable(false)
								.create();
					}

					if (!switch_call_waiting.isShowing()) {
						boolean check = reinvite(true);
						Log.d(DEBUGTAG, "click reinvite :" + check);
						if (check == true) {
							switch_call_waiting.show();

							Window win = switch_call_waiting.getWindow();
							win.setContentView(R.layout.layout_switch_waiting);
							TextView sw_call_name = (TextView) win
									.findViewById(R.id.switch_call_name);
							// sw_call_name.setText(Name);
							String sw_name = null;
							if (Name != null) {
								sw_name = Name;
							}
							sw_call_name.setText(sw_name);

							swHandler.postDelayed(WaitingTimeOut, 25000);
						}
					}

				}
			}
        };
        
        if (mSwitchBtn != null)
        	mSwitchBtn.setOnClickListener(Switchclick);
        if (qtalksetting.provisionType.equals("?type=hh")){
        	mSwitchBtn.setVisibility(View.INVISIBLE);
        	mSwitch_ll.setVisibility(View.INVISIBLE);
        }
        
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
      	getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);

		Intent intent = new Intent(this, QTReceiver.class);
		intent.setAction(QTService.ACTION_PHONEBOOK);
		intent.putExtra("pause", true);
		sendBroadcast(intent);
        
    }//end of onCreate

    @Override
    protected void onResume()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onResume- mCallHold:"+mCallHold+", mSpeakerOn:"+mSpeakerOn);

		super.onResume();
        if(Hack.isQF3a())
		    Hack.dumpAllResource(this, DEBUGTAG);
    }
    
    protected void onStart()
    {
    	super.onStart();
    }
    
    protected void onStop()
    {
    	super.onStop();
    }
    
    @Override
    protected void setSessionTime(final long session_ms) 
    {
        int seconds = (int) (session_ms / 1000);
        int minutes = seconds / 60;
        seconds     = seconds % 60;
        String str_minutes = Integer.toString(minutes);
        String str_seconds = Integer.toString(seconds);
        if (minutes<10)
            str_minutes = "0"+str_minutes;
        if (seconds < 10)
            str_seconds = "0"+str_seconds;
            
        if (mTimerTextView != null)            
            mTimerTextView.setText(str_minutes+":"+str_seconds);
    }
    
    @Override
    public void onSelfCallChange(boolean enableVideo)//Self do re-invite
    {
        //Popup AlertDialog
        // imply current re-invite status
        //createMesage("Switch to Video call",false);
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try{
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>onSelfCallChange");
                    mEndBtn.setClickable(true);
                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==onSelfCallChange");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"onSelfCallChange",err);
                }
            }
        });        
    }
    

    @Override
    public void onRemoteCallChange(final boolean enableVideo)//Remote do re-invite
    {
        //popup dialog (Switch to Video Call ?   Accept or Cancel)
        // link with UI  and suggest options
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
                    //TODO reject reinvite if 
                    //if(holdStatus)
                    //{
                    //    Log.d(DEBUGTAG,"reinvite while mute");
                    //    hangup(true);
                    //    return;
                    //}
                    Log.d(DEBUGTAG,"==>onRemoteCallChange");
                    final AlertDialog.Builder builder = new AlertDialog.Builder(InAudioSessionActivity.this);
                   // AlertDialog alert_dialog = null;
                    if(builder != null)
                    {
                    	if(alert_dialog==null){
	                    	alert_dialog  = builder.setTitle(R.string.app_name)
	                                			  .setMessage("Switch to Video call?")
	                                			  .setCancelable(false)
	                                			  .create();
                    	}
                    	if(!alert_dialog.isShowing()){
	                    	alert_dialog.show();
	                    	
	                    	Window win = alert_dialog.getWindow();  
	                        win.setContentView(R.layout.layout_switch_dialog);
	                        TextView sw_callee = (TextView) win.findViewById(R.id.switch_call_name);
	                        String remote_name = null;
	                        
	                        if(Name!=null){
	                        	remote_name = Name;
	                        }else{
	                        	remote_name = SIPNumber;
	                        }
	                        	
	                        sw_callee.setText(remote_name);
	
	                        swHandler.postDelayed(RequestTimeOut, 20000);
	                        
	                        accept_btn = (Button)win.findViewById(R.id.btn_switch_accept);  
	                        accept_btn.setOnClickListener(new OnClickListener(){  
	                            @Override  
	                            public void onClick(View v) {  
	                            	swHandler.removeCallbacks(RequestTimeOut);
	                            	swHandler.removeCallbacks(WaitingTimeOut);
	                                mEndBtn.setClickable(false);
	                                accept(enableVideo,true);   
	                            }  
	                        });  
	                          
	                        reject_btn = (Button)win.findViewById(R.id.btn_switch_cancel);  
	                        reject_btn.setOnClickListener(new OnClickListener(){  
	                            @Override  
	                            public void onClick(View v) {  
	                            	swHandler.removeCallbacks(RequestTimeOut);
	                            	swHandler.removeCallbacks(WaitingTimeOut);
	                            	hangup(true);
	                            	alert_dialog.cancel();
	                            }  
	                        });  
	                        
	                    }
                    }
                    Log.d(DEBUGTAG,"<==onRemoteCallChange");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"onRemoteCallChange",err);
                }
            }
        });
    }

    @Override
    public void onReinviteFailed()
    {
        //Somebody cancel/reject the reinvite        
        mHandler.post(new Runnable(){
            @Override
            public void run()
            {
                try
                {
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>onReinviteFailed");
                    //cleanAlertDialog();
                    	if(switch_call_waiting!=null&&switch_call_waiting.isShowing()){
                    		switch_call_waiting.dismiss();
                    		swHandler.removeCallbacks(WaitingTimeOut);
                    		
                    	}
                    	if(alert_dialog!=null&&alert_dialog.isShowing()){
                    		alert_dialog.dismiss();
                    		swHandler.removeCallbacks(RequestTimeOut);
                    	}
                    
                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==onReinviteFailed");
                }
                catch(Exception err)
                {
                    Log.e(DEBUGTAG,"onReinviteFailed",err);
                }
            }
        });
    }
    
    @Override
    public void onRemoteHangup(String message)
    {
       // finishWithDialog("Terminated");
    	RingingUtil.playSoundHangup(this);
    	AppStatus.setStatus(AppStatus.IDEL);
    	finish();
    }

    @Override
    public void onRemoteCallHold(final boolean isHeld)
    {
 
    }
    
    @Override
    public void onRequestIDR()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onRequestIDR");
    }

    private void releaseAll()
    {
    	unregisterReceiver(session_event_receiver);
    	if(switch_call_waiting!=null){
    		switch_call_waiting.dismiss();
    		switch_call_waiting = null;
    	}
    	if(alert_dialog!=null){
    		alert_dialog.dismiss();
    		alert_dialog = null;
    	}
        swHandler.removeCallbacks(WaitingTimeOut);
        swHandler.removeCallbacks(RequestTimeOut);
        mTimerTextView = null;       
        facePhoto=null;        
        mEndBtn=null;        
 //       poorNetworkAudio=null;
        inputNum=null;
        mTheClick = null;
        Name = null; 
        PhoneNumber = null;
 //       Image = null;

        timer = null;
        task = null;
        hTimer = null;
        hTask = null;
        mData = null;
        if(Hack.isQF3a())
		    Hack.dumpAllResource(this, DEBUGTAG);
    }
    
}
