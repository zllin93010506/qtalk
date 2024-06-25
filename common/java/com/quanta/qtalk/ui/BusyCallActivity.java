package com.quanta.qtalk.ui;

import java.io.File;

import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.DrawableTransfer;
import com.quanta.qtalk.util.Hack;

import android.app.Activity;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import com.quanta.qtalk.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public class BusyCallActivity extends AbstractVolumeActivity
{
//	TextView busyText1, busyText2;
	TextView busyText,callerID,callerRole;
	ImageView busyImage;
//	private String TAG="BusyCallActivity";
//	String BusyNumber, BusyType, BusyName, BusyImage;
	String BusyImage;
	Drawable Temp1 = null;
	Handler mHandler=new Handler();
	private static final String DEBUGTAG = "QTFa_BusyCallActivity";
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(com.quanta.qtalk.R.layout.layout_busycall_popupwindows);
		busyText = (TextView) findViewById(R.id.busycallText);
		callerID=(TextView)findViewById(R.id.caller_name);
		callerRole=(TextView)findViewById(R.id.roleName);
		Bundle bundle = this.getIntent().getExtras();
		if (bundle.getString("BusyType").contentEquals("BYE"))
			busyText.setText(getString(R.string.receive_bye));
		else if(bundle.getString("BusyType").contentEquals("Busy"))
			busyText.setText(getString(R.string.receive_busy));
		else
			busyText.setText(getString(R.string.no_response));
		
		BusyImage = bundle.getString("BusyImage");
		busyImage = (ImageView) findViewById(R.id.busycallImage1);
		if(Hack.isUI2F()){
			busyText.setTextAppearance(getApplicationContext(), R.style.IncomingCallID);
		}else{
			//busyText.setTextAppearance(getApplicationContext(), R.style.busyCall);
		}
		String PhoneNumber = null, Role = null, Name = null, Title = null, Role_UI=null; 
		String callerIDContent = bundle.getString("BusyID");
        QtalkDB qtalkdb = QtalkDB.getInstance(this);
    	
    	Cursor cs = qtalkdb.returnAllPBEntry();
    	int size = cs.getCount();
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"size="+size);
    	cs.moveToFirst();
        int cscnt = 0;
        for( cscnt = 0; cscnt < cs.getCount(); cscnt++)
        {
        	PhoneNumber = QtalkDB.getString(cs, "number");
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"callerIDContent="+callerIDContent+" PhoneNumber:"+PhoneNumber);
        	if (PhoneNumber.contentEquals(callerIDContent))
        	{
				Name = QtalkDB.getString(cs, "name");
				Role = QtalkDB.getString(cs, "role");
				Title = QtalkDB.getString(cs, "title");
        	}
        	cs.moveToNext();
        }
        cs.close();
        qtalkdb.close();
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
        //callerID.setText(Name);
        //callerRole.setText(Role_UI);
		Log.d(DEBUGTAG,"Role : "+Role);
		if(Role==null){
			Role = "unknown";
		}
		
		File file_temp = new File (BusyImage);
		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "file="+file_temp);
		if (file_temp.exists())
		{	
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "file exist");
			Temp1 = DrawableTransfer.LoadImageFromWebOperations(BusyImage);
			busyImage.setImageDrawable(Temp1);
		}
		else
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "file doesn't exist");
			
			if(Role.contentEquals("nurse")){
					if(Role_UI.contentEquals(getString(R.string.secreatary))){
						busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_secreatary));
	        		}else{
	        			busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_nurse));
	        		}
				}
				else if(Role.contentEquals("station")){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_nurse_station));
					
				}
				else if(Role.contentEquals("mcu")){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_broadcast_pt));
				}
				else if(Role.contentEquals("family")){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
				}
				else if(Role.contentEquals("patient")){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
				}
				else if(Role.contentEquals("staff")){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_cleaner));
				}
				else if(callerIDContent.startsWith(QtalkSettings.MCUPrefix)){
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_pushtotalk));
					busyImage.setPadding(0, 50, 0, 0);
					callerID.setText("");
					busyText.setText(getString(R.string.end_ptt));
				}
				else{
					busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.img_calling_patient));
				}
			/*if(Role.contentEquals("nurse")){
				busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.all_icn_nurse));
			}
			else if(Role.contentEquals("station")){
				busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.station));
			}
			else if(Role.contentEquals("mcu")){
				busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_broadcast));
			}
			else if(Role.contentEquals("family")){
				busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.all_icn_family));
			}
			else{
				busyImage.setImageDrawable(this.getResources().getDrawable(R.drawable.all_icn_member));
			}*/
		}
		//busyText1.setText(BusyName);
		//busyText2.setText(BusyType);
		
        final Runnable runnableThread = new Runnable() {
            public void run() {
                finish();
            }
        };
        mHandler.postDelayed(runnableThread, 2000);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
	}

	@Override
	public void onPause()
	{
		super.onPause();
	}
	
	@Override
	public void onStop()
	{
		super.onStop();
	}
	
	@Override
	public void onDestroy()
	{
		releaseAll();
		super.onDestroy();
		Activity current = ViroActivityManager.getLastActivity();
		if(ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "current:"+current);
        if(BusyCallActivity.class.isInstance(current))
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"BusyCallActivity on destroy");
            ViroActivityManager.setActivity(null);
        }
	}
	
	private void releaseAll()
    {
	/*	busyText1.setText(null);
		busyText2.setText(null);*/
	/*	busyImage.getDrawable().setCallback(null);
	//	busyImage.setImageDrawable(null);
	/*	BusyNumber=null;
		BusyType=null;
		BusyName=null;*/
		busyText.setText(null);
		if (Temp1 != null)
		{
			Temp1.setCallback(null);
			Temp1=null; 
		}
		busyImage.getDrawable().setCallback(null);
		busyImage = null;
    }
}
