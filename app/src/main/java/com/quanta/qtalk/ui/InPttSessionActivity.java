package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Log;

import java.util.ArrayList;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

public class InPttSessionActivity extends AbstractInAudioSessionActivity {

    private TextView mTimerTextView = null;
    ImageView facePhoto=null;
    Button mEndBtn=null; 
//    private static final String TAG = "InAudioSessionActivity";
    private static final String DEBUGTAG = "QTFa_InPttSessionActivity";
    //for dtmf
  //  private static final String keyValue[]={"1","2","3","4","5","6","7","8","9","*","0","#","+"};
    TextView inputNum=null;
    OnClickListener mTheClick = null;
    private String Name = null, PhoneNumber = null, Image = null;    
    
    public static final int     CONFIGURE_MENU_ITEM=Menu.FIRST;
    Timer timer;
    TimerTask task;
    Timer hTimer;
    TimerTask hTask;
    private boolean isRemoteHangup = false;
    private boolean isUserActHangup = false;
    
    private WakeLock mWakeLock = null;
    public ArrayList<Map<String, Object>> mData = new ArrayList<Map<String, Object>>();
    private BroadcastReceiver session_event_receiver = new BroadcastReceiver(){

		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(DEBUGTAG,"onReceive:"+intent.getAction());
			if(intent.getAction().equals(CommandService.HANGUP_CALL)){
				//hangup(false);
				AppStatus.setStatus(AppStatus.IDEL);
				isUserActHangup = true;
				finish();        
			}
            else if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
                //hangup(false);
				//AppStatus.setStatus(AppStatus.IDEL);
                //isUserActHangup = true;
				//finish();
                Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
			}
		}
    };
    private Handler mHandler = new Handler();
    
    final Runnable TimeOutThread = new Runnable() {
        public void run() {
        		//hangup(false);
        		isUserActHangup = true;
        		finish();
        		AppStatus.setStatus(AppStatus.IDEL);
        	}
        };
     protected void onPause() 
     {
    	 Log.d(DEBUGTAG,"==>onPause");
    	 if(!isRemoteHangup){
    		 hangup(false);
    	 }
    	 if(!isUserActHangup){
    		 finish();
    	 }
    	 super.onPause();
     };
 	private void releaseWakeLock() {
		if (null != mWakeLock) {
			mWakeLock.release();
			mWakeLock = null;
		}
	}    
     @Override
     protected void onDestroy()
     {
    	Log.d(DEBUGTAG,"==>onDestroy");
    	releaseWakeLock();
        super.onDestroy();
     	releaseAll();
     }

     @SuppressLint("Wakelock")
	@Override
     public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        Log.d(DEBUGTAG,"==>onCreate");
    	//------------------------------------QF3 HAND SET
    	AppStatus.setStatus(AppStatus.SESSION);
		IntentFilter intentfilter = new IntentFilter();
		intentfilter.addAction(CommandService.ACCEPT_CALL);
		intentfilter.addAction(CommandService.HANGUP_CALL);
		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(session_event_receiver, intentfilter); 
    	
    	//------------------------------------QF3 HAND SET
        ViroActivityManager.setActivity(this);
        //mHandler.postDelayed(TimeOutThread, 60000);
        setContentView(R.layout.layout_inpttsession);
        //////////////
        //dm = new DisplayMetrics();
        //getWindowManager().getDefaultDisplay().getMetrics(dm);
        //Caller ID======================================
        
        //--------------------------------------------------------open activity when screen on lock
        PowerManager powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK
                |PowerManager.ACQUIRE_CAUSES_WAKEUP, "qtalk:InPttSessionActivity");
        mWakeLock.acquire();
        takeKeyEvents(true);
        
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON|WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        //--------------------------------------------------------
        
        TextView callerID=(TextView)findViewById(R.id.callerName);//callerName
        String callerIDContent = mRemoteID;
        if (mContactQT!=null)
            callerIDContent=mContactQT.getDisplayName();
                //Toast.makeText(IncomingCallActivity.this,"caller="+callerIDContent,Toast.LENGTH_SHORT).show();                
        if(callerIDContent==null){
                    //Log.d(DEBUGTAG,"callerIDContent="+callerIDContent);
            callerIDContent="Unknown caller";
                    //TODO
        }        
        
        QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);
        QtalkDB qtalkdb = new QtalkDB();
    	
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
                Image = QtalkDB.getString(cs, "image");
            }
        	cs.moveToNext();
        }
        Image = Image+".jpg";
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Name:"+ Name);
        //callerID.setText(Name);
        //callerID.setText("所有護理師廣播");
        cs.close();
        qtalkdb.close();
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

        //Bottom BUTTONS======================================================================
        //int tmpAlpha=40;
        //BUTTONS=====================================
        mEndBtn=(Button)findViewById(R.id.endBtn); 
        OnClickListener theclick=new OnClickListener(){
            @Override
            public void onClick(View v){
                int selectN=v.getId();
				//check if InAudioSessionActivity is top Activity.
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"selectN:"+selectN);
                if(ViroActivityManager.isCurrentPageTopActivity(InPttSessionActivity.this,InPttSessionActivity.class.getCanonicalName()))
                {
                    switch(selectN){
       /*             	case R.id.SR2_switch_Btn:
                    		DisableAllCounter();
                    		try {
								m_service_broadcast.Start();
							} catch (RemoteException e) {
								// TODO Auto-generated catch block
								Log.e(DEBUGTAG,"",e);
							}
                    		break;
                        case R.id.videoBtn://VIDEO CALL                                
                            //TODO
                            //accept(true);//boolean enableVideo
                             //finish();
                            reinvite(true);
                            break;
                    */
                        case R.id.endBtn:
                            //hangup(false);
                        	AppStatus.setStatus(AppStatus.IDEL);
                        	isUserActHangup = true;
                            finish();                            
                            break;
                    }
                }
            }
        };
        
        if (mEndBtn != null)
            mEndBtn.setOnClickListener(theclick);
    }//end of onCreate
    
    @Override
    protected void onResume()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onResume- mCallHold:"+mCallHold+", mSpeakerOn:"+mSpeakerOn);
        super.onResume();
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
                    mEndBtn.setClickable(false);
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
                    cleanAlertDialog();
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
    	isRemoteHangup = true;
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
    	mHandler.removeCallbacks(TimeOutThread);
        mTimerTextView = null;       
        facePhoto=null;        
        mEndBtn=null;        
 //       poorNetworkAudio=null;
        inputNum=null;
        mTheClick = null;
        Name = null; 
        PhoneNumber = null;
        Image = null;

        timer = null;
        task = null;
        hTimer = null;
        hTask = null;
        mData = null;
    }
}
