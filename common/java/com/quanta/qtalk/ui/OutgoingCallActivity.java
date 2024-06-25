package com.quanta.qtalk.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
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
import com.quanta.qtalk.util.QtalkUtility;

public class OutgoingCallActivity extends AbstractOutgoingCallActivity
    {
        /*
        boolean downFlag=false;
        ViewGroup vg;        
        int DOWN_BORDER;
        int initfaceX;
        int initfaceY;
        */
    	private BroadcastReceiver session_event_receiver = new BroadcastReceiver(){

    		@Override
    		public void onReceive(Context context, Intent intent) {
    			Log.d(DEBUGTAG,"onReceive:"+intent.getAction());
    			if(intent.getAction().equals(CommandService.ACCEPT_CALL)){
    				
    			}else if(intent.getAction().equals(CommandService.HANGUP_CALL)){
    				hangup();
    				finish();
    			}
				else if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
					//hangup();
					//finish();
					Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
				}
    		}
        };
        DisplayMetrics dm;
        int screenH;
        private ImageView facePhoto=null;
        private static final String DEBUGTAG = "QSE_OutgoingCallActivity";
        private String Name = null, Alias = null, PhoneNumber = null/*, Image = null*/, Role = null, Title = null, Role_UI = null;
        Drawable Temp1 = null;
        TextView callerID;
        TextView callerRole;
        TextView CallingName;
        Button endcall;

		QtalkEngine engine;
		QtalkSettings qtalksetting ;


        @Override
        public void onResume()
        {
        	Log.d(DEBUGTAG,"==>onResume "+this.hashCode());
            super.onResume();
			Intent intent = new Intent();
	        intent.setAction(LauncherReceiver.ACTION_VC_MAKE_CALL_CB);
	        sendBroadcast(intent);
            Log.d(DEBUGTAG,"<==onResume "+this.hashCode());
        }
        @Override
        protected void onPause() {
        	Log.d(DEBUGTAG,"==>onPause "+this.hashCode());
        	super.onPause();
        	Log.d(DEBUGTAG,"<==onPause "+this.hashCode());
        }
        @Override
        public void onCreate(Bundle savedInstanceState)
        {
        	Log.d(DEBUGTAG,"==>onCreate "+this.hashCode()+" taskID=>"+this.getTaskId());

			qtalksetting = QtalkUtility.getSettings(this, engine);
        	//------------------------------------QF3 HAND SET
        	AppStatus.setStatus(AppStatus.OUTGOING);
    		IntentFilter intentfilter = new IntentFilter();
    		intentfilter.addAction(CommandService.ACCEPT_CALL);
    		intentfilter.addAction(CommandService.HANGUP_CALL);
    		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
            registerReceiver(session_event_receiver, intentfilter); 
        	
        	//------------------------------------QF3 HAND SET
            super.onCreate(savedInstanceState);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
			if(Hack.isQF3a())
				Hack.dumpAllResource(this, DEBUGTAG);
            Log.d(DEBUGTAG,"<==onCreate "+this.hashCode());
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
        	Log.d(DEBUGTAG,"==>onRemoteHangup "+this.hashCode());
        	AppStatus.setStatus(AppStatus.IDEL);
        	if(QtalkSettings.nurseCall){
        		
        		CommandService.shouldNextNurseCall = true;
        		Log.d(DEBUGTAG,"==>shouldNextNurseCall "+CommandService.shouldNextNurseCall);
        	}else{
        		Log.d(DEBUGTAG,"finish");
        		finish();
        	}
            //finishWithDialog(msg, false);
            Log.d(DEBUGTAG,"<==onRemoteHangup "+this.hashCode());
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
        
        @Override
        protected void onDestroy()
        {
        	Log.d(DEBUGTAG,"==>onDestroy "+this.hashCode());
        	super.onDestroy();
        	releaseAll();
        	Log.d(DEBUGTAG,"<==onDestroy "+this.hashCode());
        }
        
        private void releaseAll()
        {
        	unregisterReceiver(session_event_receiver);
        	if (facePhoto != null) {
        		facePhoto.getDrawable().setCallback(null);
            	facePhoto=null;
        	}
//        	myfacePhoto=null;
        	Name=null;
            PhoneNumber=null;
 //           Image=null;
            if (Temp1 != null)
            {
	            Temp1.setCallback(null);
	            Temp1=null;
            }
            dm=null;
            callerID=null;
            callerRole=null;
        }
        @Override
        protected void onStart() {
        	Log.d(DEBUGTAG,"==>onStart :"+this.hashCode());
        	super.onStart();
            ViroActivityManager.setActivity(this);
            Flag.InsessionFlag.setState(true);
            setContentView(R.layout.layout_outgoingcall);            
            ///////////
            dm = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(dm);        
            //===get caller ID====================================================
            callerID=(TextView)findViewById(R.id.callerName);
            callerRole=(TextView)findViewById(R.id.roleName);
//            CallingName=(TextView)findViewById(R.id.facetext);
            String callerIDContent=super.mRemoteID;
            if (super.mContactQT!=null)
                callerIDContent=super.mContactQT.getDisplayName();
            //Toast.makeText(IncomingCallActivity.this,"caller="+callerIDContent,Toast.LENGTH_SHORT).show();
            if(callerIDContent==null){
                callerIDContent="Unknown caller";
            }

            QtalkDB qtalkdb = QtalkDB.getInstance(this);
        	
        	Cursor cs = qtalkdb.returnAllPBEntry();
        	
        	int size = cs.getCount();
        	Log.d(DEBUGTAG,"qtalkdb size: "+size);
        	
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
	        if(Title!=null){
	            if(Name == null){
	                name_phrase = callerIDContent;
	            }else if (Name.length() > max_str_len){
	                name_phrase = Name.substring(0, max_str_len) + "...";
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
					if (Name.length() > max_str_len) {
						name_phrase = Name.substring(0, max_str_len) + "...";
					} else
						name_phrase = Name;
		            callerID.setText(name_phrase);
		            callerRole.setText(Role_UI);
		            Log.d(DEBUGTAG, "Role_UI:"+Role_UI+", name:"+name_phrase);
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

	        Log.d(DEBUGTAG,"name: "+ Name);	        
            facePhoto=(ImageView)findViewById(R.id.faceiconPic);
 
            if(Role==null){
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
				}
				else if(Role.contentEquals("staff")){
					facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_cleaner));
				}           
				else{
					facePhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
				}           

                    endcall=(Button)findViewById(R.id.endBtn);                    
                    OnClickListener theclick=new OnClickListener(){
                        @Override
                        public void onClick(View v){
                        	QtalkSettings.nurseCall = false;
                            hangup();
                            finish();
                        }
                    };
                    endcall.setOnClickListener(theclick);
                    
        	Log.d(DEBUGTAG,"<==onStart :"+this.hashCode());
        }
        @Override
        protected void onNewIntent(Intent intent) {
        	Log.d(DEBUGTAG,"==>onNewIntent :"+this.hashCode()+", taskID=>"+this.getTaskId());
        	QTReceiver.unregisterActivity(mCallID, this);
        	super.onNewIntent(intent);
        	Bundle bundle = intent.getExtras();
        	this.onCreate(bundle);
        	this.onStart();
        	Log.d(DEBUGTAG,"<==onNewIntent :"+this.hashCode());
        }
        
        
    }
