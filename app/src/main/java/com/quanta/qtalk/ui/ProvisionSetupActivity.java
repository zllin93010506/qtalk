package com.quanta.qtalk.ui;

import java.util.Calendar;

//import android.app.AlertDialog;
import android.app.DatePickerDialog;
import android.app.Dialog;
import android.content.pm.PackageManager; 
import android.content.Context;
//import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
//import android.content.pm.ActivityInfo;
//import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;
//import android.view.Menu;
//import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TableRow;
import android.widget.TextView;
//import android.widget.Toast;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.VersionDefinition;
import com.quanta.qtalk.provision.PackageChecker;
import com.quanta.qtalk.provision.PackageChecker.AppInfo;
import com.quanta.qtalk.provision.PackageVersionUtility;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public class ProvisionSetupActivity extends AbstractSetupActivity
{
	private static final String DEBUGTAG = "QSE_ProvisionSetupActivity";
	
	public static final boolean debugMode = true;//switch 
	LinearLayout mFullLayoutSetup = null;
	EditText mEdtUserName = null;
    TextView mEdtPassWord = null;
    EditText mEdtCSServer = null;
    TextView mTxtMessage = null;
    TextView tv_http_cnt_result=null;
    TableRow ServerTable = null;
    Spinner  mTypeSpinner = null;
    ImageView Login_Icon = null;
    Button saveBtn = null;
    private Button mPickDate;
    static final int DATE_DIALOG_ID = 0;
    private int mYear=0, mMonth=0, mDay=0;
    String mmMonth=null,mmDay=null;
    public static final String MESSAGE = "MESSAGE";
    private String[] loginType = {"STATION", "PHONE", "PATIENT"};
    private String type = null;
    private int TouchNumber = 0;
    private boolean ServerVisible = false;
    private long touchTimeStart = 0, touchTimeEnd = 0;
    private TextView versionName;

    AdapterView.OnItemSelectedListener spinnerlistener = new AdapterView.OnItemSelectedListener() {
    	  @Override
    	  public void onItemSelected(AdapterView adapterView, View view,
    	    int position, long id) {
    	    switch (adapterView.getSelectedItemPosition()) {
    	    case 0:
    	    	type = "?type=ns";
    	    	break;
    	    case 1:
    	    	type = "?type=qtfa";
    	    	break;
    	    case 2:
    	    	type = "?type=hh";
    	    	break;
    	    default:
    	    	break;
    	  }
    	  }
    	  @Override
    	  public void onNothingSelected(AdapterView arg0) {
    	  }
    	 };
    //touch space and hide soft keyboard
    public void CloseKeyBoard() {
    	mEdtUserName.clearFocus();
//    	mEdtCSServer.clearFocus();
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(mEdtUserName.getWindowToken(), 0);
//        imm.hideSoftInputFromWindow(mEdtCSServer.getWindowToken(), 0);
    }
  //touch space and hide soft keyboard
    public boolean onTouchEvent(MotionEvent event) {
        CloseKeyBoard();
      /*  switch (event.getAction()) {  
        case MotionEvent.ACTION_DOWN: {
        		float X = event.getX();
        		float Y = event.getY();
        		touchTimeEnd = System.currentTimeMillis();
        		if (X < 500 && Y < 500)
        		{
        			if (touchTimeEnd - touchTimeStart < 500 || TouchNumber < 1)
        			{
        				touchTimeStart = touchTimeEnd;
        				TouchNumber = TouchNumber +1;
        			}
        			else
        			{
        				TouchNumber = 0;
        			}
        		}
	        	if(debugMode) Log.d(DEBUGTAG," TouchNumber="+TouchNumber);
	            if (TouchNumber > 20)
	            {
	            	ServerVisible = true;
	            	TouchNumber = 0;
	            	updateDisplay();
	            }
	            break;  
	         }
        }*/
        return super.onTouchEvent(event);
    }
    
    @Override
    public void onCreate(Bundle savedInstanceState){
    	super.onCreate(savedInstanceState);
        
        setContentView(R.layout.layout_provision_setting);

        mFullLayoutSetup = (LinearLayout) findViewById(R.id.FullLayoutSetup);
        mEdtUserName = (EditText) findViewById(R.id.mEdtUserName);
        mEdtCSServer = (EditText) findViewById(R.id.mEdtCSServer);
        mTypeSpinner = (Spinner) findViewById(R.id.mTypespinner);
        ServerTable = (TableRow) findViewById(R.id.TableRow04);
        Login_Icon = (ImageView) findViewById(R.id.login_Icon);
        
        Login_Icon.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v) {
            	touchTimeEnd = System.currentTimeMillis();
    			if (touchTimeEnd - touchTimeStart < 500 || TouchNumber < 1)
    			{
    				touchTimeStart = touchTimeEnd;
    				TouchNumber = TouchNumber +1;
    			}
    			else
    			{
    				TouchNumber = 0;
    			}
	        	if(debugMode) Log.d(DEBUGTAG," TouchNumber="+TouchNumber);
	            if (TouchNumber > 8)
	            {
	            	ServerVisible = true;
	            	TouchNumber = 0;
	            	updateDisplay();
	            }
            }
        });
        
        mEdtUserName.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    InputMethodManager imm = (InputMethodManager) v
                            .getContext().getSystemService(
                                    Context.INPUT_METHOD_SERVICE);
                    if (imm.isActive()) {
                        imm.hideSoftInputFromWindow(
                                v.getApplicationWindowToken(), 0);
                    }
                    return true;
                }
                return false;
            }
        });
        
        mEdtCSServer.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    InputMethodManager imm = (InputMethodManager) v
                            .getContext().getSystemService(
                                    Context.INPUT_METHOD_SERVICE);
                    if (imm.isActive()) {
                        imm.hideSoftInputFromWindow(
                                v.getApplicationWindowToken(), 0);
                    }
                    return true;
                }
                return false;
            }
        });
        ArrayAdapter typeList = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, loginType);                                     
        mTypeSpinner.setAdapter(typeList);
        mTypeSpinner.setOnItemSelectedListener(spinnerlistener);
        
        
        mEdtPassWord = (TextView) findViewById(R.id.mEdtPassWord);
      //  mDateDisplay = (TextView) findViewById(R.id.mEdtPassWord);
        mPickDate = (Button) findViewById(R.id.myDatePickerButton);
        mTxtMessage = (TextView) findViewById(R.id.mTxtMessage);
        mPickDate.setOnClickListener(new View.OnClickListener() {
            @SuppressWarnings("deprecation")
			public void onClick(View v) {
                showDialog(DATE_DIALOG_ID);
            }
        });
        
        //get the current date
        final Calendar c = Calendar.getInstance();
        mYear = c.get(Calendar.YEAR);
        mMonth = c.get(Calendar.MONTH);
        mDay = c.get(Calendar.DAY_OF_MONTH);        
        updateDisplay();
        
        saveBtn=(Button)findViewById(R.id.saveButton);
        saveBtn.setOnClickListener(new Button.OnClickListener()
        {
            @Override
            public void onClick(View v){
                QtalkSettings settings = null;
                Intent myintent = new Intent();

                try
                {   
                    settings = ProvisionSetupActivity.super.getSetting(false);
                    settings.provisionID = mEdtUserName.getText().toString();
                    settings.provisionPWD = mEdtPassWord.getText().toString();
                    settings.provisionServer = "172.16.182.75/thc/dis";
                    settings.provisionCameraSwitch = true;
                    settings.provisionCameraSwitchReason = 3;
                    if (ServerVisible){
                        settings.provisionServer = mEdtCSServer.getText().toString()+"/thc/dis";
                        settings.provisionType = type;
                        if(type.contentEquals("?type=ns")||type.contentEquals("?type=qtfa")) {
                            settings.provisionCameraSwitch = true;
                            settings.provisionCameraSwitchReason = 4;
                        }
                        else {
                            settings.provisionCameraSwitch = false;
                            settings.provisionCameraSwitchReason = 5;
                        }
                    }

                    if(settings.provisionServer.startsWith("https://"))
                    	settings.provisionServer = settings.provisionServer;
                    else
                    	settings.provisionServer = "https://"+settings.provisionServer;
                     
                    if(QTReceiver.engine(ProvisionSetupActivity.this, false).isLogon()) {    
                    	QTReceiver.engine(ProvisionSetupActivity.this, false).logout("");
                    }
                    ProvisionSetupActivity.super.updateSetting(settings, false);
                    
                    //Below code cause a PorvosionSteupActivity Loop 
                    Log.d(DEBUGTAG,"settings.provisionID: "+settings.provisionID+", settings.provisionPWD: "+settings.provisionPWD);
                    myintent.setClass(ProvisionSetupActivity.this,com.quanta.qtalk.ui.ProvisionLoginActivity.class);
                    startActivity(new Intent(getApplicationContext(), com.quanta.qtalk.ui.ProvisionLoginActivity.class));
                     
                } 
                catch (FailedOperateException err)
                {
                    Log.e(DEBUGTAG,settings==null?"":"CS_ID:"+settings.provisionID +", CS_PASSWORD:"+settings.provisionPWD +", CS_SERVER:"+settings.provisionServer, err);
                }
                	 
                finish(); 
                 
            }
        });
        
        
     /*   final Button cancelBtn=(Button)findViewById(R.id.cancelButton);
        cancelBtn.setOnClickListener(new Button.OnClickListener(){
            @Override
            public void onClick(View v){
                QTReceiver.finishQT(ProvisionSetupActivity.this);
                finish();
            }     
        });*/
        versionName = (TextView) findViewById(R.id.versionName);
    	String version=null;
		int versionCode=0;
		try {
			version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
			versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
			versionName.setText("QSPT-VC "+version);
		} catch (NameNotFoundException e1) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"getVersion",e1);
			Log.e(DEBUGTAG,"",e1);
		}
		Hack.startNursingApp(this);
    }
    
    private void updateDisplay() {
    	
    	if (mMonth != 0)
    	{
	    	mmMonth = ""+mMonth;
	        mmDay = ""+mDay;
	        if(debugMode) Log.d(DEBUGTAG, "mmMonth="+mmMonth+"     mmDay="+mmDay);
	        
	        if (mMonth < 10)
	        	mmMonth = "0"+(mMonth);
	        if (mDay < 10)
	        	mmDay = "0"+(mmDay);
	        
	        this.mEdtPassWord.setText(
	            new StringBuilder()
	                    // Month is 0 based so add 1
	                    .append(mmMonth).append(mmDay).append(mYear));
    	}
    	if (ServerVisible)
    	{
    		ServerTable.setVisibility(View.VISIBLE);
    	}
    }
    
    private DatePickerDialog.OnDateSetListener mDateSetListener =
    	    new DatePickerDialog.OnDateSetListener() {
    	        public void onDateSet(DatePicker view, int year, 
    	                              int monthOfYear, int dayOfMonth) {
    	            mYear = year;
    	            mMonth = monthOfYear+1;
    	            mDay = dayOfMonth;
    	            mPickDate.setText(new StringBuilder().append(mYear).append("-").append(mMonth).append("-").append(mDay));
    	            updateDisplay();
    	        }
    	    };
    	    
    	    
   @Override
    protected Dialog onCreateDialog(int id) {
    	       switch (id) {
    	       case DATE_DIALOG_ID:
    	          return new DatePickerDialog(this,
    	                    mDateSetListener,
    	                    mYear, mMonth, mDay);
    	       }
    	       return null;
    	    }
    
    protected void onDestroy() 
    { 
        super.onDestroy();
        if(mFullLayoutSetup!=null)
        	mFullLayoutSetup.removeAllViews();
        releaseAll();
    }
    
    private void releaseAll()
    {
    	mEdtUserName = null;
        mEdtPassWord = null;
        mEdtCSServer = null;
        mTxtMessage = null;
        tv_http_cnt_result=null;
        mPickDate = null;
        mmMonth = null;
        mmDay = null;
    }
    
    @Override
    public void onPause()
    {
        super.onPause();
        finish();
       
    }
	@Override
    public void onResume()
    {	
        super.onResume();
       
        StringBuilder sb = new StringBuilder();
        AppInfo qt_info = PackageChecker.getInstalledQTInfo(this);
        String qt_version = qt_info.getVersionName()+"."+qt_info.getVersionCode();
        sb.append(qt_info.getAppName());
        sb.append(":");
        sb.append(qt_version);

//        try
//        {    
//        	boolean X264Installed;
//        	X264Installed = PackageChecker.isX264Installed(this);
//        	if(debugMode) Log.d(DEBUGTAG,"X264Installed="+X264Installed);
//        	if (X264Installed == true)
//        	{
//        		if(debugMode) Log.d(DEBUGTAG,"X264Installed=true");
//	            AppInfo x264_info = PackageChecker.getInstalledX264Info(this);
//	            String x264_version = x264_info.getVersionName()+"."+x264_info.getVersionCode();
//	            sb.append(" ");
//	            sb.append(x264_info.getAppName());
//	            sb.append(":");
//	            sb.append(x264_version);
//        	}
//        	else
//        	{
//        		PackageChecker.CheckInstallX264(this);
//        		//finish();
//        	}
//        }catch(Exception e){
//        	Log.e(DEBUGTAG, "writeLog",e);
//        }
        try {
            QtalkSettings settings = ProvisionSetupActivity.super.getSetting(false);
           
            if(settings.provisionID != null)
            {	   
                mEdtUserName.setText(settings.provisionID.toString());
            }
            if(settings.provisionPWD != null)
            {	   
                mEdtPassWord.setText(settings.provisionPWD.toString());
            }
            /*if(settings.provisionServer != null)
            {    
                //Log.d(DEBUGTAG, "ProvisionServer:"+settings.ProvisionServer.toString());
                String server = settings.provisionServer;
                
                if(server.trim().length() > 0){
                	//server = server.substring(0, server.indexOf("/"));
                  }  
                mEdtCSServer.setText(server);
                Log.d(DEBUGTAG, "Line 296: "+server);
            }*/
            settings.signinMode = VersionDefinition.PROVISION;
            settings.versionNumber = PackageVersionUtility.getQTInfo(ProvisionSetupActivity.this).getVersionName() + "."+PackageVersionUtility.getQTInfo(ProvisionSetupActivity.this).getVersionCode();
            if(debugMode) Log.d(DEBUGTAG, "versionNumber="+settings.versionNumber);
            ProvisionSetupActivity.super.updateSetting(settings, false);
        } catch (FailedOperateException e) {
        	Log.e(DEBUGTAG,"getSetting",e);
            //Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }

    }
	
	@Override
	 public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		 if (keyCode == KeyEvent.KEYCODE_BACK ) 
	        {
	        	System.exit(0);
	        }	        
		 return super.onKeyDown(keyCode, event);
	 }
}
