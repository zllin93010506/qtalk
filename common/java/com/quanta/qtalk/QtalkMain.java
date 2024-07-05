package com.quanta.qtalk;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.widget.ListView;

import com.quanta.qtalk.provision.PackageVersionUtility;
import com.quanta.qtalk.ui.AbstractSetupActivity;
import com.quanta.qtalk.ui.ContactsActivity;
import com.quanta.qtalk.ui.HistoryActivity;
import com.quanta.qtalk.ui.InVideoSessionActivity;
import com.quanta.qtalk.ui.LauncherReceiver;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.QTReceiver;
import com.quanta.qtalk.ui.QTService;
import com.quanta.qtalk.ui.QThProvisionUtility;
import com.quanta.qtalk.ui.RingingUtil;
import com.quanta.qtalk.ui.ViroActivityManager;
import com.quanta.qtalk.ui.history.HistoryDataBaseUtility;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

import java.io.File;

public class QtalkMain extends AbstractSetupActivity
{
    private ListView listCats;
    private String catsName[]={
            "Setup",
            "Dialer",
            "Incoming call",
            "Outgoing call",
            "Video call in-session",
            "Voice call in-session",
            "Welcome page"
    };
    
    private static final boolean RELEASE = true;
    private static final String DEBUGTAG = "QSE_QtalkMain";
    protected final Handler mHandler = new Handler();
    private static Context mcontext;
    public static boolean isDeactivate = false;  //解決換床位  停止phonebook check
    AlertDialog mAlertDialog = null;
    HistoryDataBaseUtility historydatabase = null;
  
    public static final String KEY_REMOTE_ID            = "REMOTE_ID";
    public static final String KEY_CALL_ID              = "CALL_ID";
    public static final String KEY_CALL_TYPE            = "CALL_TYPE";
    public static final String KEY_ENABLE_VIDEO         = "ENABLE_VIDEO";
    
    public static final String KEY_AUDIO_DEST_IP        = "AUDIO_DEST_IP";
    public static final String KEY_AUDIO_LOCAL_PORT     = "AUDIO_LOCAL_PORT";
    public static final String KEY_AUDIO_DEST_PORT      = "AUDIO_DEST_PORT";
    public static final String KEY_AUDIO_PAYLOAD_TYPE   = "AUDIO_PAYLOAD_TYPE";
    public static final String KEY_AUDIO_SAMPLE_RATE    = "AUDIO_SAMPLERATE";
    public static final String KEY_AUDIO_BITRATE        = "AUDIO_BITRATE";
    public static final String KEY_AUDIO_MIME_TYPE      = "AUDIO_MIMETYPE";
    public static final String KEY_AUDIO_SSRC           = "AUDIO_SSRC";

    public static final String KEY_VIDEO_DEST_IP        = "VIDEO_DEST_IP";
    public static final String KEY_VIDEO_LOCAL_PORT     = "VIDEO_LOCAL_PORT";
    public static final String KEY_VIDEO_DEST_PORT      = "VIDEO_DEST_PORT";
    public static final String KEY_VIDEO_PAYLOAD_TYPE   = "VIDEO_PAYLOAD_TYPE";
    public static final String KEY_VIDEO_SAMPLE_RATE    = "VIDEO_SAMPLERATE";
    public static final String KEY_VIDEO_MIME_TYPE      = "VIDEO_MIMETYPE";
    public static final String KEY_VIDEO_SSRC           = "VIDEO_SSRC";
    
    public static final String KEY_CAMERA_INDEX         = "CAMERA_INDEX"; 
    public static final String KEY_SESSION_START_TIME   = "SESSION_START_TIME";
    public static final String KEY_PRODUCT_TYPE         = "PRODUCT_TYPE";
    public static final String KEY_CALL_HOLD            = "CALL_HOLD";
    public static final String KEY_CALL_MUTE            = "CALL_MUTE";
    public static final String KEY_CALL_ONHOLD          = "CALL_ONHOLD";
    public static final String KEY_SPEAKER_ON           = "SPEAKER_ON";
    
    public static final String KEY_VIDEO_WIDTH          = "VIDEO_WIDTH";
    public static final String KEY_VIDEO_HEIGHT         = "VIDEO_HEIGHT";
    public static final String KEY_VIDEO_FPS            = "VIDEO_FPS";
    public static final String KEY_VIDEO_KBPS           = "VIDEO_KBPS";
    public static final String KEY_ENABLE_PLI           = "ENABLE_PLI";
    public static final String KEY_ENABLE_FIR           = "ENABLE_FIR";
    public static final String KEY_ENABLE_TEL_EVENT     = "ENABLE_TEL_EVENT";
    
    public static final String KEY_AUDIO_FEC_CTRL				= "AUDIO_FEC_CTRL";		//eason - FEC info
    public static final String KEY_AUDIO_FEC_SEND_PTYPE		= "AUDIO_FEC_SEND_PTYPE";
    public static final String KEY_AUDIO_FEC_RECV_PTYPE		= "AUDIO_FEC_RECV_PTYPE";
    public static final String KEY_VIDEO_FEC_CTRL				= "VIDEO_FEC_CTRL";
    public static final String KEY_VIDEO_FEC_SEND_PTYPE		= "VIDEO_FEC_SEND_PTYPE";
    public static final String KEY_VIDEO_FEC_RECV_PTYPE		= "VIDEO_FEC_RECV_PTYPE";

    
    public static final String KEY_ENALBE_AUDIO_SRTP		    = "ENALBE_AUDIO_SRTP";    //Iron - SRTP info
    public static final String KEY_AUDIO_SRTP_SEND_KEY		= "AUDIO_SRTP_SEND_KEY";
    public static final String KEY_AUDIO_SRTP_RECEIVE_KEY		= "AUDIO_SRTP_RECEIVE_KEY";
    public static final String KEY_ENALBE_VIDEO_SRTP		    = "ENALBE_VIDEO_SRTP";
    public static final String KEY_VIDEO_SRTP_SEND_KEY		= "VIDEO_SRTP_SEND_KEY";
    public static final String KEY_VIDEO_SRTP_RECEIVE_KEY		= "VIDEO_SRTP_RECEIVE_KEY";
	public void loopbackTest(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(DEBUGTAG,"onCreate "+this.hashCode());
		
		mcontext = this;
		if(QtalkSettings.ScreenSize<0) {
			QtalkUtility.initRatios(this);
		}
		setContentView(R.layout.main);
		  
         final Intent myintent = new Intent();
         myintent.setClass(QtalkMain.this,QTReceiver.class);
         myintent.setClass(QtalkMain.this,LauncherReceiver.class);
         myintent.setAction(QtalkAction.ACTION_APP_START);
         sendBroadcast(myintent);
         String string = PackageVersionUtility.getVersion(this);
         Log.d(DEBUGTAG, string);
         
         RingingUtil Ring = new RingingUtil();
         Ring.init(this);
         Ring.playSoundOutgoingingCall(this);
         try {
        	 Thread.sleep(2500);
         } catch (InterruptedException e) {
        	 Log.e(DEBUGTAG,"",e);
         }
         Ring.stop();
         
         Log.d(DEBUGTAG, "myIntent.setAction(\"com.quanta.qtalk.ui.videosession\")");
         Bundle mybundle = new Bundle();
         mybundle.putString(KEY_CALL_ID              , "0");
         mybundle.putInt(KEY_CALL_TYPE               , 2);
         mybundle.putString(KEY_REMOTE_ID            , "0");
         mybundle.putBoolean(KEY_ENABLE_VIDEO        , true);
         mybundle.putInt(KEY_PRODUCT_TYPE            , 0);
         mybundle.putLong(KEY_SESSION_START_TIME     , 1000);
         mybundle.putBoolean(KEY_ENABLE_TEL_EVENT    , false);          
         // audio
         mybundle.putString(KEY_AUDIO_DEST_IP    , "127.0.0.1");
         mybundle.putInt(KEY_AUDIO_LOCAL_PORT    , 6666);
         mybundle.putInt(KEY_AUDIO_DEST_PORT     , 6666);
         mybundle.putInt(KEY_AUDIO_PAYLOAD_TYPE  , 111);
         mybundle.putInt(KEY_AUDIO_SAMPLE_RATE   , 16000);
         mybundle.putInt(KEY_AUDIO_BITRATE       , 24000);
         mybundle.putString(KEY_AUDIO_MIME_TYPE  , "opus");
         mybundle.putInt(KEY_AUDIO_FEC_CTRL		 , 1);
         mybundle.putInt(KEY_AUDIO_FEC_SEND_PTYPE	, 127);
         mybundle.putInt(KEY_AUDIO_FEC_RECV_PTYPE	, 127);
         mybundle.putBoolean(KEY_ENALBE_AUDIO_SRTP     , false);
         mybundle.putString(KEY_AUDIO_SRTP_SEND_KEY    , "abcdefg");
         mybundle.putString(KEY_AUDIO_SRTP_RECEIVE_KEY , "abcdefg");
         // video
         mybundle.putString(KEY_VIDEO_DEST_IP    , "127.0.0.1"); // "172.23.4.207"
         mybundle.putInt(KEY_VIDEO_LOCAL_PORT    , 8888);
         mybundle.putInt(KEY_VIDEO_DEST_PORT     , 8888);
         mybundle.putInt(KEY_VIDEO_PAYLOAD_TYPE  , 109);
         mybundle.putInt(KEY_VIDEO_SAMPLE_RATE   , 90000);
         mybundle.putString(KEY_VIDEO_MIME_TYPE  , "h264");
         mybundle.putInt(KEY_VIDEO_WIDTH         , 1280);
         mybundle.putInt(KEY_VIDEO_HEIGHT        , 720);
         mybundle.putInt(KEY_VIDEO_FPS           , 30);
         mybundle.putInt(KEY_VIDEO_KBPS          , 5000);
         mybundle.putBoolean(KEY_ENABLE_PLI      , true);
         mybundle.putBoolean(KEY_ENABLE_FIR      , true);
         mybundle.putInt(KEY_VIDEO_FEC_CTRL		 , 1);
         mybundle.putInt(KEY_VIDEO_FEC_SEND_PTYPE	, 127);
         mybundle.putInt(KEY_VIDEO_FEC_RECV_PTYPE	, 127);
         mybundle.putBoolean(KEY_ENALBE_VIDEO_SRTP     , false);
         mybundle.putString(KEY_VIDEO_SRTP_SEND_KEY    , "abcdefg");
         mybundle.putString(KEY_VIDEO_SRTP_RECEIVE_KEY , "abcdefg");

         Intent myIntent = new Intent();
         myIntent.setClass(QtalkMain.this, InVideoSessionActivity.class);
         myIntent.putExtras(mybundle);
         startActivity(myIntent);
         finish();
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {

    	if (RELEASE) {
	        super.onCreate(savedInstanceState);
	        overridePendingTransition(0, 0);
	        RingingUtil RU;
	        String open_history = null;
	        
	        Log.d(DEBUGTAG,"onCreate "+this.hashCode());
	        RU = new RingingUtil();
	        RU.init(this);
	        mcontext = this;
		    
		    Bundle bundle = this.getIntent().getExtras();
	        if(bundle!=null){
	        	open_history = bundle.getString("KEY_OPEN_HISTORY");
	        }
	        if(QtalkSettings.ScreenSize<0){
	        	QtalkUtility.initRatios(this);
	        }
	        setContentView(R.layout.main);
	
	        final Intent myintent=new Intent();
	        myintent.setClass(QtalkMain.this,QTReceiver.class);
	        myintent.setClass(QtalkMain.this,LauncherReceiver.class);
	        myintent.setAction(QtalkAction.ACTION_APP_START);
	        sendBroadcast(myintent);
	        
	        String string = PackageVersionUtility.getVersion(this);
	        if(ProvisionSetupActivity.debugMode) Log.l(DEBUGTAG, string);
            Hack.init(this);
        	String provision = Hack.getStoragePath()+"provision.xml";
        	File dirFile = new File(provision);
            QtalkEngine my_engine = QTReceiver.engine(this, false);
            Log.d(DEBUGTAG,"my_engine:"+my_engine+", my_engine.isLogon():"+my_engine.isLogon()+", Flag.Activation.getState():"+Flag.Activation.getState() + ", dirFile.exists():"+dirFile.exists());

            if ((my_engine!=null && my_engine.isLogon()) || (dirFile.exists())) 
            {
                boolean startQTService = (QThProvisionUtility.provision_info !=null);

                startQTService = (dirFile.exists() || startQTService);

            	if(startQTService)
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        startForegroundService(new Intent(this,QTService.class));
                    }

                Intent intent = new Intent();
                if(open_history == null) {
                	if(ViroActivityManager.getLastActivity()!=null){
                		Log.d(DEBUGTAG,"Last Activity:"+ViroActivityManager.getLastActivity().getLocalClassName());
                	}
                	intent.setClass(QtalkMain.this, ContactsActivity.class);
                }
                else{
                	if(open_history.contentEquals("NO")) {
                		intent.setClass(QtalkMain.this, ContactsActivity.class);
                	}
                	else {
                		intent.setClass(QtalkMain.this, HistoryActivity.class);
                	}                	
                }
                intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                startActivity(intent);
                finish();
                overridePendingTransition(0, 0);
            }
        }
        else {
        	loopbackTest(savedInstanceState);
        }
        
    } //End of public void onCreate


	@Override
    public void onDestroy()
    {
		Log.d(DEBUGTAG,"onDestroy "+this.hashCode()+", start");
    	super.onDestroy();
        if (!RELEASE) {
            stopService(new Intent(this,QTService.class));
        }
        Log.d(DEBUGTAG,"onDestroy "+this.hashCode()+", end");
    }
    
    @Override 
    protected void onPause()
    {
        super.onPause();
        Log.d(DEBUGTAG, "onPause "+this.hashCode());
    }
    protected void onResume() 
    {  	
    	Log.d(DEBUGTAG, "onResume "+this.hashCode());
    	super.onResume();
    }   
    
    public void clearApplicationData() {
    	 File cache = getCacheDir();
    	 File appDir = new File(cache.getParent());
    	 if(appDir.exists()){
    	  String[] children = appDir.list();
    	  for(String s : children){
    	   if(!s.equals("lib")){
    	    if(deleteDir(new File(appDir, s))){
    	     }
    	   }
    	   }
    	  }
    	}
    	public static boolean deleteDir(File dir) {
    	    if (dir != null && dir.isDirectory()) {
    	        String[] children = dir.list();
    	        for (int i = 0; i < children.length; i++) {
    	            boolean success = deleteDir(new File(dir, children[i]));
    	            if (!success) {
    	                return false;
    	            }
    	        }
    	    }
    	    return dir.delete();
    	}
}
 
