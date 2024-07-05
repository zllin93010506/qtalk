package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.ActionBar.LayoutParams;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Typeface;
import android.graphics.drawable.AnimationDrawable;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.view.GestureDetector.OnGestureListener;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.quanta.qtalk.AppStatus;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.media.video.CameraSourcePlus;
import com.quanta.qtalk.media.video.IVideoStreamReportCB;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;


public class InVideoSessionActivity extends AbstractInVideoSessionActivity implements OnGestureListener {
	private static final String DEBUGTAG = "QSE_InVideoSessionActivity";
	RelativeLayout                  mLayout=null;
	boolean                         localViewDrag=false;
	private boolean                 isScreenPortrait=true;
	static final int                PIP_TL=1;
	static final int                PIP_TR=2;
	static final int                PIP_BL=3;
	static final int                PIP_BR=4;
	int                             pipPosition = 0;
	int                             swapStatus=1;
	int                             menuIconSize=75;
	public  int orientation =0;
	private TextView                mTimerTextView = null;
	private TextView                mLowbandTextView =null;
	private SurfaceView             mCameraView = null;
	private View  mLocalView = null;
	private SurfaceView             mRemoteView = null;
	private RelativeLayout            mCameraViewLayout = null;
	private RelativeLayout         mControlLayout  = null;
	private RelativeLayout			mTitleBarLayout = null;
	private RelativeLayout         mLocalViewLayout = null;

	private RelativeLayout            mRemoteViewLayout = null;
	private RelativeLayout          mVideosessionbar = null;
	private LinearLayout            mVideosessionbarContainer = null;// ViewGroup RelativeLayout
	private RelativeLayout          endcallContainer = null;//for CHAT/SHARE switch button
	RelativeLayout.LayoutParams     swapP=null;
	RelativeLayout.LayoutParams     swapPFish=null;
	LinearLayout.LayoutParams       endlp=null;
	public  RelativeLayout           mLowbandwidth= null;
	public  RelativeLayout           mHoldLayout = null;
	public ImageView                mShareBtn = null;
	Button                      mEndCallBtn=null;
	ToggleButton                      mMuteBtn = null;
	ToggleButton                      mSpeakerBtn = null;
	private RelativeLayout      mEndCall_rl = null;
	private RelativeLayout      mScaleRemote_rl = null;
	ToggleButton                 mScaleRemoteBtn = null;
	private Boolean             isFullScreenNow = false;
	private ImageView             mInitVideoConnect = null;
	AnimationDrawable 		mConnectAnimation =null;

	ImageView                       mToAudioBtn=null;
	/*    ImageView                       mMuteBtn=null;
        ImageView                       mDTMFBtn=null;*/
	ImageView                       tmpLocal;
	ImageView 						mLocalBG;
	ImageView                       tmpRemote;
	ImageButton 					vIcon = null;
	ImageView                       titleIcon = null;
	ImageView						vImageCover = null;
	private boolean 				VideoEnable = true;
	LayoutInflater                  li=null;
	private int initRemoteW = 0, initRemoteH = 0;
	private View decorView;
	Handler delay = null;
	private String Name = null, PhoneNumber = null, Role = null/*, Image = null*/;
//    private TextView callerID;

	TextView sr2_switchcountdown_tv;
	ListView sr2_switchpx_list;
	final static int SR2_SWITCH_COUNTDOWN = 8;
	int countdownlimit = SR2_SWITCH_COUNTDOWN;
	Timer timer;
	TimerTask task;
	Timer hTimer;
	TimerTask hTask;
	public ArrayList<Map<String, Object>> mData = new ArrayList<Map<String, Object>>();
	OrientationEventListener myOrientationEventListener;
	Configuration mNewConfig = null;
	int mRotate = 0;
	int RemoteViewW = 0, RemoteViewH = 0;
	int LocalViewW = 0, LocalViewH = 0;
	int cameraW = 0, cameraH = 0, cameraBackgroundH = 0, cameraBackgroundW = 0;
	int LocalViewMarginB = 0;
	int LandEndcallMargin = 0;
	float x_scale=1.0f, y_scale=1.0f;
	int ScreenSize =0 ;
	int screenW = 0;
	int screenH = 0;
	int navigationbar_height = 0;
	int navigationbar_width = 0;
	float remoteRatio = 3.0f/4.0f;
	float localRatio = 3.0f/4.0f;

	private boolean isShowBar = false;
	private boolean isShowScaleBtn = false;

	private int LocalViewStartX = 0,LocalViewStartY =0;

	public static float scale = 0;
	public static final int LOWBANDWIDTH_OFF = 0;
	public static final int LOWBANDWIDTH_ON = 1;
	public static final int UPDATE_REMOTE_RESULOTION = 2;
	public static final int UPDATE_LOCAL_RESULOTION = 3;
	public static final int SHOW_REMOTE_SCALE_BTN = 4;
	public static final int SEND_IDR_REQUEST = 5;
	public static final int NO_STEAM_HANGUP = 6;
	public static final String REMOTEVIEW_WIDTH = "REMOTEVIEW_WIDTH";
	public static final String REMOTEVIEW_HEIGHT = "REMOTEVIEW_HEIGHT";
	public static final String LOCALVIEW_WIDTH = "LOCALVIEW_WIDTH";
	public static final String LOCALVIEW_HEIGHT = "LOCALVIEW_HEIGHT";
	public static final String SHOW_REMOTE_SCALE = "SHOW_REMOTE_SCALE";

	Bitmap muteBmp = null;
	Bitmap reSizeMuteBmp = null;
	ImageView muteimg = null;
	ImageView holdimg = null;
	boolean isholdnow = false;
	boolean localview_moving = false;
	int localview_width = 0, localview_height = 0;
	private boolean firstopen = true;
	private boolean isRemoteHangup = false;
	private boolean isUserActHangup = false;

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
	@SuppressLint("HandlerLeak")
	protected Handler mHandler = new Handler(){
		public void handleMessage(Message msg) {
			Log.d(DEBUGTAG,"Msg.what:"+msg.what);
			switch(msg.what)
			{
				case LOWBANDWIDTH_OFF:
					if(mLowbandwidth!=null){
						mLowbandwidth.setVisibility(View.INVISIBLE);
					}
					break;
				case LOWBANDWIDTH_ON:
					if(mLowbandwidth!=null){
						if(!isholdnow){
							mLowbandwidth.setVisibility(View.VISIBLE);
							mLowbandwidth.bringToFront();
						}
					}
					break;
				case UPDATE_REMOTE_RESULOTION:
					if(msg.getData()!=null){
						int width = msg.getData().getInt(REMOTEVIEW_WIDTH, 640);
						int height = msg.getData().getInt(REMOTEVIEW_HEIGHT, 480);
						initRemoteW = width;
						initRemoteH = height;
						if(mScaleRemoteBtn!=null){
							if(mScaleRemoteBtn.isChecked()){
								UpdateRemoteResolution((screenH)*initRemoteW/initRemoteH, screenH);
							}else{
								UpdateRemoteResolution(initRemoteW, initRemoteH);
							}
						}
					}
					break;
				case UPDATE_LOCAL_RESULOTION:
					if(msg.getData()!=null){
						int width = msg.getData().getInt(LOCALVIEW_WIDTH,640);
						int height = msg.getData().getInt(LOCALVIEW_HEIGHT,480);
						UpdateLocalResolution(width,height);
					}
					break;
				case SHOW_REMOTE_SCALE_BTN:
					if(msg.getData()!=null){
						isShowScaleBtn = msg.getData().getBoolean(SHOW_REMOTE_SCALE, false);
						if(ScreenSize==4){
							if(isShowScaleBtn)
								mScaleRemote_rl.setVisibility(View.VISIBLE);
							else
								mScaleRemote_rl.setVisibility(View.GONE);
						}else{
							isShowScaleBtn = false;
						}
					}
					break;
				case SEND_IDR_REQUEST:
					onDemandIDR();
					break;
				case NO_STEAM_HANGUP:
					AppStatus.setStatus(AppStatus.IDEL);
					isUserActHangup = true;
					finish();
					break;
			}


		};
	};

	private void UpdateRemoteResolution(int width, int height)
	{
		Log.d(DEBUGTAG,"Update Remote Resolution, width:"+width+" height:"+height);
		float ratio = (float)height / (float)width;
		if(mRemoteView!=null){
			//mRemoteViewLayout.removeView(mRemoteView);
			ViewGroup.LayoutParams lp = mRemoteView.getLayoutParams();
			ViewGroup.LayoutParams lp2 = holdimg.getLayoutParams();
			if(ScreenSize==4){
				if(width>height){
					if(width <=640) {
						lp.width = screenW/2;
						lp.height = (int)((float)lp.width * ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}else if(ratio>=0.74){ //3:4
						lp.height = screenH;
						lp.width = (int)((float)lp.height/ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}else{ //9:16
						lp.width = screenW-2;
						lp.height = screenH;
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}
				}else{
					if(width <=640) {
						Log.d(DEBUGTAG,"Update Remote Resolution XXXX");
						lp.height = screenW/2;
						lp.width = (int)((float)lp.height / ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}else if(ratio>=1.33){ //3:4
						lp.height = screenH;
						lp.width = (int)((float)lp.height/ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}
				}

			}else{
				if(isScreenPortrait){
					if(width>height){
						lp.width = screenW;
						lp.height = (int)((float)lp.width*ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}else{
						lp.width = screenW;
						lp.height = (int)((float)lp.width*ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}
				}else{
					if(width>height){
						lp.height = screenH;
						lp.width = (int)((float)lp.height/ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}else{
						lp.height = screenH;
						lp.width = (int)((float)lp.height/ratio);
						RemoteViewW = lp.width;
						RemoteViewH = lp.height;
					}
				}
				Log.d(DEBUGTAG,"xxxx  isScreenPortrait:"+isScreenPortrait+", width:"+width+", height"+height+
						", screenW:"+screenW+", screenH:"+screenH+", lp.width:"+lp.width+", lp.height:"+lp.height);
				remoteRatio = ratio;
			}
			mRemoteView.setLayoutParams(lp);
			lp2.width = RemoteViewW;
			lp2.height = RemoteViewH;
			holdimg.setLayoutParams(lp2);

			if(mConnectAnimation!=null){
				mConnectAnimation.stop();
			}
			if(mInitVideoConnect!=null){
				mInitVideoConnect.removeCallbacks(init_video_animation);
				mInitVideoConnect.setVisibility(View.INVISIBLE);
			}

		}
	}
	private void UpdateLocalResolution(int width, int height)
	{
		Log.d(DEBUGTAG,"[Localview] UpdateLocalResolution(), width:"+width+", height:"+height);
		if(mLocalViewLayout!=null){
			if(ScreenSize==4){
				float ratio = (float)height / (float)width;
				ViewGroup.LayoutParams lp = mLocalViewLayout.getLayoutParams();

				lp.height = screenH/10*3;
				lp.width = (int)((float)lp.height / ratio);
				mLocalViewLayout.setLayoutParams(lp);
			}
		}else{
			Log.d(DEBUGTAG,"local view is null");
		}
	}

	private void DisableAllCounter()
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
	}

//    private GestureDetector mGestureDetector;

	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		Log.d(DEBUGTAG,"==>onConfigurationChanged");
		if (newConfig == null) {
			return ;
		}
		//if(newConfig.orientation==Configuration.ORIENTATION_PORTRAIT){
		if(Hack.isPhone()) {
			isScreenPortrait=true;
		}else{
			isScreenPortrait=false;
		}
		if(mVideosessionbarContainer!=null){
		}
		super.onConfigurationChanged(newConfig);
		//RESET slidemenu
		mNewConfig = newConfig;
		//initGUI();
		resetGUI();
		//ShowNavigationBar(3000);
		Log.d(DEBUGTAG,"<==onConfigurationChanged");
	}

	@Override
	protected void onPause()
	{
		Date now = Calendar.getInstance().getTime();
		if((!isRemoteHangup)&&(!isUserActHangup)) {
			if (isTaskRoot() || ((onResumeTime != null) && (Math.abs(now.getTime() - onResumeTime.getTime()) < 1000))) {
				Log.d(DEBUGTAG, "onTop or change state from onResume too fast, skip do nPause.");
				skipOnPause = true;
				super.onPause();
				return;
			}
		}
		skipOnPause = false;
		Log.d(DEBUGTAG,"onPause "+this.hashCode()+" ,isRemoteHangup:"+isRemoteHangup+", isUserActHangup:"+isUserActHangup);
		if(!isRemoteHangup){
			hangup(false);

			if(!isUserActHangup){
				finish();
			}
		}
		super.onPause();
	}
	Date onResumeTime = null;
	protected void onResume()
	{
		Log.d(DEBUGTAG,"onResume "+this.hashCode());
		super.onResume();
		if(skipOnPause = true)
			return;
		if(firstopen){
			firstopen = false;
		}else{
			ShowNavigationBar(3000);
			initGUI();
		}
		if(Hack.isQF3a())
			Hack.dumpAllResource(this, DEBUGTAG);
		Calendar calendar = Calendar.getInstance();
		calendar.add(Calendar.SECOND, 1);
		onResumeTime = calendar.getTime();
	}
	QtalkSettings qtalksetting ;
	QtalkEngine engine;
	@Override
	protected void onDestroy()
	{
		Log.d(DEBUGTAG, "onDestroy "+this.hashCode()+", start");
		Intent intent = new Intent(this, QTReceiver.class);
		intent.setAction(QTService.ACTION_PHONEBOOK);
		intent.putExtra("pause", false);
		sendBroadcast(intent);
		super.onDestroy();
		//myOrientationEventListener.disable();
		//releaseAll();
		qtalksetting = QtalkUtility.getSettings(this, engine);
		if(qtalksetting.provisionType.contentEquals("?type=ns")) {
			Log.d(DEBUGTAG, "onDestroy nurse_station_tap_selected=" + QtalkSettings.nurse_station_tap_selected);
			switch (QtalkSettings.nurse_station_tap_selected) {
				case 1:
					Intent contact = new Intent();
					contact.setClass(this, ContactsActivity.class);
					startActivity(contact);
					overridePendingTransition(0, 0);
					//Log.d(DEBUGTAG, "onDestroy ContactsActivity intent");
					break;
				case 2:
					Intent history = new Intent();
					history.setClass(this, HistoryActivity.class);
					startActivity(history);
					overridePendingTransition(0, 0);
					//Log.d(DEBUGTAG, "onDestroy HistoryActivity intent");
					break;
				default:
					break;
			}
		}
		if(Hack.isQF3a())
			Hack.dumpAllResource(this, DEBUGTAG);
		Log.d(DEBUGTAG, "onDestroy "+this.hashCode()+", end");
	}

	Runnable hide_navigation = new Runnable() {

		@SuppressLint("InlinedApi")
		@Override
		public void run() {
			Log.d(DEBUGTAG,"HIDE NAVIGATION");
			if(mHandler!=null){
				mHandler.removeCallbacks(hide_navigation);
				mHandler.removeCallbacks(hide_others);
				mHandler.postDelayed(hide_others, 500);
			}
			decorView = getWindow().getDecorView();
			decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
					| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
					| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_FULLSCREEN
					| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
			isShowBar =false;
		}
	};
	Runnable init_video_animation = new Runnable(){
		@Override
		public void run() {
			if(mConnectAnimation!=null)
				mConnectAnimation.start();
		}
	};
	Runnable show_navigation = new Runnable() {

		@SuppressLint("InlinedApi")
		@Override
		public void run() {
			if(mHandler!=null){
				mHandler.removeCallbacks(hide_navigation);
				mHandler.removeCallbacks(hide_others);
				mHandler.postDelayed(hide_navigation, 3000);
			}
			decorView = getWindow().getDecorView();
			decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
					| View.SYSTEM_UI_FLAG_LAYOUT_STABLE
					| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

			if(mControlLayout!=null){
				mControlLayout.setVisibility(View.VISIBLE);
			}
			isShowBar = true;
		}
	};
	Runnable hide_others = new Runnable() {

		@Override
		public void run() {
			if((mControlLayout!=null) && (!QtalkSettings.control_keep_visible)){
				mControlLayout.setVisibility(View.INVISIBLE);
			}
		}

	};
	private Bitmap getReSizeVideoMuteBmp(){
		int width = muteBmp.getWidth();
		int height = muteBmp.getHeight();
		int newWidth = 0;
		int newHeight = 0;
		if(isScreenPortrait){
			newWidth = 480;
			newHeight = 640;
		}else{
			newWidth = 640;
			newHeight = 480;
		}

		float scaleWidth = ((float) newWidth) / width;
		float scaleHeight = ((float) newHeight) / height;
		Matrix mtx = new Matrix();
		mtx.postScale(scaleWidth, scaleHeight);
		return Bitmap.createBitmap(muteBmp, 0, 0, width, height, mtx, true);

	}
	@SuppressLint("InlinedApi")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		Log.d(DEBUGTAG, "onCreate "+this.hashCode());
		//------------------------------------QF3 HAND SET
		AppStatus.setStatus(AppStatus.SESSION);
		IntentFilter intentfilter = new IntentFilter();
		intentfilter.addAction(CommandService.ACCEPT_CALL);
		intentfilter.addAction(CommandService.HANGUP_CALL);
		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
		registerReceiver(session_event_receiver, intentfilter);

		//------------------------------------QF3 HAND SET
		ShowNavigationBar(0);
		//force to landscape
		//this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
			setContentView(R.layout.layout_invideosession_land_k72);
		}else if(Hack.isQF3a()||Hack.isQF5()|| Hack.isStation()||Hack.isPT()) {
			setContentView(R.layout.layout_invideosession_land_k72);
		}else{
			//if(this.getResources().getConfiguration().orientation==this.getResources().getConfiguration().ORIENTATION_PORTRAIT){
			if(Hack.isPhone()){
				setContentView(R.layout.layout_invideosession_port);
				isScreenPortrait = true;
				Log.d(DEBUGTAG,"isScreenPortrait:"+isScreenPortrait);
			}
			else{
				setContentView(R.layout.layout_invideosession_land);
				isScreenPortrait = false;
				Log.d(DEBUGTAG,"isScreenPortrait:"+isScreenPortrait);
			}
		}
		myOrientationEventListener = new OrientationEventListener(this, SensorManager.SENSOR_DELAY_NORMAL){

			@Override
			public void onOrientationChanged(int angle) {
				int rotate = getWindowManager().getDefaultDisplay().getRotation();
				if (mRotate != rotate) {
					Log.d(DEBUGTAG, "=>InVideoSessionActivity onOrientationChanged mRotate = " + mRotate + " rotate = " + rotate);
					if (Math.abs(mRotate - rotate) == 2){
						//onConfigurationChanged(mNewConfig);
						if (mMediaEngine!=null)
							mMediaEngine.setDisplayRotation(getWindowManager().getDefaultDisplay().getRotation());
					}
					mRotate = rotate;
				}
			}};


		if (myOrientationEventListener.canDetectOrientation()){
			myOrientationEventListener.enable();
		}
		else{
			//finish();
		}

		super.onCreate(savedInstanceState);
		mVideoRender.setHandler(mHandler);
		ViroActivityManager.setActivity(this);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);

		mRemoteView = getRemoteView();
		if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
			mLocalView = getH264View(mHandler);
		}
		else {
			mLocalView = getLocalView();
		}

		if(mCameraView == null) {
			mCameraView = getCameraView();
		}

		initGUI();

		String callerIDContent = mRemoteID;
		if (mContactQT!=null)
			callerIDContent=mContactQT.getDisplayName();
		if(callerIDContent==null){
			callerIDContent="Unknown caller";
		}

		setVideoReportListener((IVideoStreamReportCB)InVideoSessionActivity.this);
		if(mShareBtn != null && Hack.isSamsungTablet())
			mShareBtn.setVisibility(View.INVISIBLE);
		else
		{
			//set orientation
			CameraSourcePlus.setEnableCameraRotation(true);
		}

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED  
            | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD  
            | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON  
            | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);

		Intent intent = new Intent(this, QTReceiver.class);
		intent.setAction(QTService.ACTION_PHONEBOOK);
		intent.putExtra("pause", true);
		sendBroadcast(intent);
	}//End of onCreate
	@SuppressLint("NewApi")
	private void computeScreenSize()
	{
		final DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getRealMetrics(dm);
		scale = this.getResources().getDisplayMetrics().density;
		QtalkUtility.initRatios(this);
		screenW = QtalkUtility.screenWidth;
		screenH =QtalkUtility.screenHeight;

		if(screenH > screenW) isScreenPortrait = true;
		else isScreenPortrait = false;
	}

	public void resetGUI()
	{
		computeScreenSize();
		mRemoteViewLayout.setGravity(Gravity.CENTER_VERTICAL);

		if (isScreenPortrait == true) //
		{
			muteimg.setBackground(getResources().getDrawable(R.drawable.bg_vc_audio_vga2));

			//===Put remoteView==================================================
			UpdateRemoteResolution(RemoteViewW,RemoteViewH);
			LocalViewH = screenH/4;
			LocalViewW = LocalViewH*3/4;

			if(mLocalView!=null)
			{
				RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
						LocalViewW,
						LocalViewH);
				LocalViewStartX = screenW-LocalViewW-screenW/10;
				LocalViewStartY = screenH-LocalViewH-screenH/10;
				Log.d(DEBUGTAG,"screenW:"+screenW+", screenH:"+screenH+", LocalViewStartX:"+LocalViewStartX+", LocalViewStartY:"+LocalViewStartY);
				lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
				if(mLocalView instanceof SurfaceView){
					((SurfaceView) mLocalView).setZOrderOnTop(true);
					((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
				}else{
					mLocalView.bringToFront();
				}
				if(mEnableCameraMute){
					muteimg.setVisibility(View.VISIBLE);
					muteimg.bringToFront();
					Log.d(DEBUGTAG,"muteimg VISIBLE");
				}else{
					muteimg.setVisibility(View.INVISIBLE);
					Log.d(DEBUGTAG,"muteimg INVISIBLE");
				}
				mLocalViewLayout.setTranslationX(0.0f);
				mLocalViewLayout.setTranslationY(0.0f);
				mLocalViewLayout.setLayoutParams(lp);
				if(mEnableCameraMute)
					mLocalView.setBackgroundColor(Color.BLACK);
			}
			if(mLocalView instanceof SurfaceView)  //front the remote view but behind the control button
				((SurfaceView) mLocalView).setZOrderMediaOverlay(true);

			RelativeLayout.LayoutParams lp2 = new RelativeLayout.LayoutParams(
					LayoutParams.WRAP_CONTENT,
					LayoutParams.WRAP_CONTENT);
			int pixels = (int) (100*scale+0.5f);
			lp2.setMargins(0, 0, 0, pixels);
			lp2.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			lp2.addRule(RelativeLayout.CENTER_HORIZONTAL);
			mControlLayout.setLayoutParams(lp2);
		}
		else {//isScreenPortrait == true
			muteimg.setBackground(getResources().getDrawable(R.drawable.bg_vc_audio_vga));

			//===Put remoteView==================================================
			UpdateRemoteResolution(RemoteViewW,RemoteViewH);
			LocalViewW = screenW/4;
			LocalViewH = LocalViewW*3/4;
			if(mLocalView!=null)
			{
				RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
						LocalViewW,
						LocalViewH);
				if(RemoteViewW>RemoteViewH)
					LocalViewStartX = screenW-((screenW-RemoteViewW)/2)-LocalViewW;
				else
					LocalViewStartX = screenW-LocalViewW-screenW/10;
				LocalViewStartY = screenH-LocalViewH;
				lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
				if(mLocalView instanceof SurfaceView){
					((SurfaceView) mLocalView).setZOrderOnTop(true);
					((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
				}else{
					mLocalView.bringToFront();
				}
				if(mEnableCameraMute){
					muteimg.setVisibility(View.VISIBLE);
					muteimg.bringToFront();
					Log.d(DEBUGTAG,"muteimg VISIBLE 1");
				}else{
					muteimg.setVisibility(View.INVISIBLE);
					Log.d(DEBUGTAG,"muteimg INVISIBLE 1");
				}
				mLocalViewLayout.setTranslationX(0.0f);
				mLocalViewLayout.setTranslationY(0.0f);
				mLocalViewLayout.setLayoutParams(lp);

			}//mLocalView!=null
			if(mLocalView instanceof SurfaceView)  //front the remote view but behind the control button
				((SurfaceView) mLocalView).setZOrderMediaOverlay(true);

			RelativeLayout.LayoutParams lp2 = new RelativeLayout.LayoutParams(
					LayoutParams.WRAP_CONTENT,
					LayoutParams.WRAP_CONTENT);
			int pixels = (int) (20*scale+0.5f);
			lp2.setMargins(0, 0, 0, pixels);
			lp2.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			lp2.addRule(RelativeLayout.CENTER_HORIZONTAL);
			mControlLayout.setLayoutParams(lp2);
		}

		Log.d(DEBUGTAG, "resetGUI(), screen:"+screenW+"x"+screenH+
				", localview:"+LocalViewW+"x"+LocalViewH+
				", remoteview:"+RemoteViewW+"x"+RemoteViewH+
				", ScreenSize:"+ScreenSize+
				", isScreenPortrait:"+isScreenPortrait);
	}
	private static final int MIN_DELAY_TIME= 500;  // 两次点击间隔不能少于500ms
	private static long lastClickTime;

	public static boolean isFastClick() {
		boolean flag = true;
		long currentClickTime = System.currentTimeMillis();
		if ((currentClickTime - lastClickTime) >= MIN_DELAY_TIME) {
			flag = false;
			lastClickTime = currentClickTime;
		}
		return flag;
	}

	private void initButtonListener()
	{
		mEndCallBtn = (Button)findViewById(R.id.mEndCallBtn);
		mEndCall_rl = (RelativeLayout)findViewById(R.id.end_call_rl);
		mEndCallBtn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AppStatus.setStatus(AppStatus.IDEL);
				isUserActHangup = true;
				finish();
			}
		});

		mMuteBtn = (ToggleButton)findViewById(R.id.mCameraMuteBtn);
		if(mEnableCameraMute) mMuteBtn.setChecked(true);
		else mMuteBtn.setChecked(false);
		mMuteBtn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				synchronized(InVideoSessionActivity.this) {
					if(mMuteBtn.isChecked()) {
						Log.d(DEBUGTAG,"Set Video Mute: True");

						muteimg.setVisibility(View.VISIBLE);
						Log.d(DEBUGTAG,"muteimg VISIBLE 2");
						muteimg.bringToFront();
						mEnableCameraMute = true;

						if((Hack.isK72() || Hack.isQF3()) && QtalkSettings.enable1832) {
							setVideoMute(true, reSizeMuteBmp);
							//synchronized(InVideoSessionActivity.this) {
							if(mLocalView == null)
								return;

							setVideoMute(true, reSizeMuteBmp);
							mLocalViewLayout.removeView(mLocalView);
							mLocalView.invalidate();
							mLocalView = null;
							//}
						}
						// Frank ++
						else if(Hack.isQF3a()||Hack.isQF5()) {
							//synchronized(InVideoSessionActivity.this) {
							if(mLocalView == null)
								return;

							setVideoMute(true, reSizeMuteBmp);
							mLocalViewLayout.removeView(mLocalView);
							mLocalView.invalidate();
							mLocalView = null;
							//}
						}
						// Frank --
						else {
							setVideoMute(true, reSizeMuteBmp);
							mLocalView.setBackgroundColor(Color.BLACK);
						}
					}
					else {
						Log.d(DEBUGTAG,"Set Video Mute: False");
						muteimg.setVisibility(View.INVISIBLE);
						setVideoMute(false, reSizeMuteBmp);
						mEnableCameraMute = false;

						if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832) {
//						setVideoMute(false, reSizeMuteBmp);
//						mEnableCameraMute = false;
							//synchronized(InVideoSessionActivity.this) {
							if(mLocalView != null)
								return;
							mLocalView = getH264View(mHandler);
							try {
								mLocalViewLayout.addView(mLocalView);
							}
							catch(java.lang.IllegalStateException ex) {
								Log.e(DEBUGTAG,"",ex);
							}
							//}
						}
						// Frank ++
						else if(Hack.isQF3a()||Hack.isQF5()) {
							//synchronized(InVideoSessionActivity.this) {
							if(mLocalView != null) {
								mLocalViewLayout.removeView(mLocalView);
								mLocalView.invalidate();
								mLocalView = null;
							}
							mLocalView = getLocalView();
							if(mLocalView != null)
							{
								RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LocalViewW, LocalViewH);
								LocalViewStartX = (screenW)/20;
								LocalViewStartY = (screenH)*3/5;
								lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
								if(mLocalView instanceof SurfaceView){
									((SurfaceView) mLocalView).setZOrderOnTop(true);
									((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
								}
								else {
									mLocalView.bringToFront();
								}
								try {
									mLocalViewLayout.addView(mLocalView);
								}
								catch(java.lang.IllegalStateException ex) {
									Log.e(DEBUGTAG,"",ex);
								}
								mLocalViewLayout.setLayoutParams(lp);
							}
							//}
//						setVideoMute(false, reSizeMuteBmp);
//						mEnableCameraMute = false;
						}
						else {
//						setVideoMute(false, reSizeMuteBmp);
//						mEnableCameraMute = false;
						}
						// Frank --


						mHandler.postDelayed(new Runnable() {
							@Override
							public void run() {
								//synchronized(InVideoSessionActivity.this) {
								if(mLocalView != null) {
									mLocalView.setBackgroundColor(Color.TRANSPARENT);
									mLocalView.bringToFront();
								}
								//}
							}
						}, 500);
/*
					mHandler.postDelayed(new Runnable() {
						@Override
						public void run() {
							synchronized(InVideoSessionActivity.this) {
								if(mMuteBtn.isChecked()) {
									muteimg.setVisibility(View.INVISIBLE);
								}
							}
						}
					}, 50);
*/
					}

					if(mCameraView!=null)
						mCameraView.invalidate();
					//synchronized(InVideoSessionActivity.this) {
					if(mLocalView!=null)
						mLocalView.invalidate();
					//}
				}
			}
		});

		mSpeakerBtn = (ToggleButton)findViewById(R.id.mSpeakerBtn);
		mSpeakerBtn.setChecked(true);
		mSpeakerBtn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if(mSpeakerBtn.isChecked()){
					enableLoudSpeaker(true);
				}else{
					enableLoudSpeaker(false);
				}
			}
		});

		mScaleRemoteBtn = (ToggleButton)findViewById(R.id.mScaleRemotBtn);
		mScaleRemote_rl = (RelativeLayout)findViewById(R.id.scaleRemote);
		mScaleRemoteBtn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if(initRemoteW!=0 && initRemoteH!=0){
					if(mScaleRemoteBtn.isChecked()){
						UpdateRemoteResolution((screenH)*initRemoteW/initRemoteH, screenH);
					}else{
						UpdateRemoteResolution(initRemoteW, initRemoteH);
					}
				}
			}
		});
	}


	private void initLocalView()
	{
		int muteimage_drawable = 0;
		if(isScreenPortrait) muteimage_drawable = R.drawable.bg_vc_audio_vga2;
		else muteimage_drawable = R.drawable.bg_vc_audio_vga;
		muteBmp = Hack.getBitmap(this, muteimage_drawable);
		reSizeMuteBmp = getReSizeVideoMuteBmp();
		muteimg = new ImageView(this);
		holdimg = new ImageView(this);
		if((Hack.isK72() || (Hack.isQF3()) && QtalkSettings.enable1832)){
			muteimg.setBackground(getResources().getDrawable(R.drawable.bg_vc_audio_720p));
			holdimg.setBackground(getResources().getDrawable(R.drawable.bg_vc_holdon_720p));
		}
//        else if(Hack.isQF3a()) {
//            muteimg.setBackgroundResource(R.drawable.bg_vc_audio_720p);
//            holdimg.setBackgroundResource(R.drawable.bg_vc_holdon_720p);
//        }
		else{
			muteimg.setBackground(getResources().getDrawable(muteimage_drawable));
			holdimg.setBackground(getResources().getDrawable(R.drawable.bg_vc_holdon_vga));
		}

		if (isScreenPortrait == true) {
			mLocalViewLayout = (RelativeLayout)mLayout.findViewById(R.id.local_view_ll);
			mLocalViewLayout.setOnTouchListener(new OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
					localview_moving = true;
					return false;
				}
			});

			if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
				LocalViewW = screenW/8*2;
				LocalViewH = LocalViewW*9/16;
			}
//            else if(Hack.isQF3a()){
//            	LocalViewW = screenW/8*2;
//            	LocalViewH = LocalViewW*9/16;
//            }
			else {
				LocalViewH = screenH/8*2;
				LocalViewW = LocalViewH*3/4;
			}
			//synchronized(InVideoSessionActivity.this) {
			if(mLocalView!=null)
			{
				RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LocalViewW, LocalViewH);
				LocalViewStartX = screenW-LocalViewW-screenW/10;
				LocalViewStartY = screenH-LocalViewH-screenH/10;
				lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
				if(mLocalView instanceof SurfaceView){
					((SurfaceView) mLocalView).setZOrderOnTop(true);
					((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
				}
				else{
					mLocalView.bringToFront();
				}
				LinearLayout.LayoutParams mute_lp = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
				mLocalViewLayout.addView(muteimg, mute_lp);
				mLocalViewLayout.addView(mLocalView);
				if(mEnableCameraMute) {
					muteimg.setVisibility(View.VISIBLE);
					muteimg.bringToFront();
					Log.d(DEBUGTAG,"muteimg VISIBLE 3");
				}
				else {
					muteimg.setVisibility(View.INVISIBLE);
					Log.d(DEBUGTAG,"muteimg INVISIBLE 3");
				}
				mLocalViewLayout.setLayoutParams(lp);
			}//if(mLocalView!=null)
			else {
				tmpLocal=new ImageView(this);
				tmpLocal.setImageResource(R.drawable.local_camera);
				LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LocalViewW, LocalViewH);
				mLocalViewLayout.addView(tmpLocal,lp);
			}
			//}
			if(mLocalView instanceof SurfaceView)  //front the remote view but behind the control button
				((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
		}//if (isScreenPortrait == true)
		else {
			if(ScreenSize < 4) {
				mLocalViewLayout = (RelativeLayout)mLayout.findViewById(R.id.local_view_ll);
				mLocalViewLayout.setOnTouchListener(new OnTouchListener() {
					@Override
					public boolean onTouch(View v, MotionEvent event) {
						localview_moving = true;
						return false;
					}
				});

				LocalViewW = screenW/4;
				LocalViewH = LocalViewW * 3/4;
				//synchronized(InVideoSessionActivity.this) {
				if(mLocalView!=null) {
					RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LocalViewW, LocalViewH);
					if(RemoteViewW>RemoteViewH)	LocalViewStartX = screenW-((screenW-RemoteViewW)/2)-LocalViewW;
					else LocalViewStartX = screenW-LocalViewW-screenW/10;

					LocalViewStartY = screenH-LocalViewH;
					lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
					if(mLocalView instanceof SurfaceView){
						((SurfaceView) mLocalView).setZOrderOnTop(true);
						((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
					}
					else{
						mLocalView.bringToFront();
					}
					LinearLayout.LayoutParams mute_lp = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
					mLocalViewLayout.addView(muteimg, mute_lp);
					try {
						mLocalViewLayout.addView(mLocalView);
					}
					catch(java.lang.IllegalStateException ex) {
						Log.e(DEBUGTAG,"",ex);
					}
					if(mEnableCameraMute){
						muteimg.setVisibility(View.VISIBLE);
						muteimg.bringToFront();
						Log.d(DEBUGTAG,"muteimg VISIBLE 4");
					}
					else{
						muteimg.setVisibility(View.INVISIBLE);
						Log.d(DEBUGTAG,"muteimg INVISIBLE 4");
					}
					mLocalViewLayout.setLayoutParams(lp);
				}
				else {
					tmpLocal=new ImageView(this);
					tmpLocal.setImageResource(R.drawable.local_camera);
					LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LocalViewW, LocalViewH);
					mLocalViewLayout.addView(tmpLocal,lp);
				}
				//}
				if(mLocalView instanceof SurfaceView)  //front the remote view but behind the control button
					((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
			}
			else { /* ScreenSize >= 4 */
				mLocalViewLayout = (RelativeLayout)mLayout.findViewById(R.id.local_view_ll);
				mLocalViewLayout.setOnTouchListener(new OnTouchListener() {
					@Override
					public boolean onTouch(View v, MotionEvent event) {
						localview_moving = true;
						return false;
					}
				});

				if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
					LocalViewW = screenW/8*2;
					LocalViewH = LocalViewW*9/16;
				}
				// FIXME: if enable QF3a() condition will let QF3a() only output 10fps.
				//        [when happen 10fps case, no trigger SurfaceChange() to restart]
//                else if(Hack.isQF3a()){
//                	LocalViewW = screenW/8*2;
//                	LocalViewH = LocalViewW*9/16;
//                }
				else{
					LocalViewW = screenW/8*2;
					LocalViewH = LocalViewW*3/4;
				}
				//synchronized(InVideoSessionActivity.this) {
				if(mLocalView != null)
				{
					RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LocalViewW, LocalViewH);
					LocalViewStartX = (screenW)/20;
					LocalViewStartY = (screenH)*3/5;
					lp.setMargins(LocalViewStartX, LocalViewStartY, 0, 0);
					if(mLocalView instanceof SurfaceView){
						((SurfaceView) mLocalView).setZOrderOnTop(true);
						((SurfaceView) mLocalView).setZOrderMediaOverlay(true);
					}
					else {
						mLocalView.bringToFront();
					}
					mLocalViewLayout.addView(muteimg, new RelativeLayout.LayoutParams(LocalViewW, LocalViewH));
					mLocalViewLayout.addView(mLocalView);
					if(mEnableCameraMute){
						muteimg.setVisibility(View.VISIBLE);
						muteimg.bringToFront();
						if(Hack.isQF3a()||Hack.isQF5()) {
							if (mLocalView != null) {
								mLocalViewLayout.removeView(mLocalView);
								mLocalView.invalidate();
								mLocalView = null;
							}
						}
						Log.d(DEBUGTAG,"muteimg VISIBLE 5");
					}
					else {
						muteimg.setVisibility(View.INVISIBLE);
						Log.d(DEBUGTAG,"muteimg INVISIBLE 5");
					}
					mLocalViewLayout.setLayoutParams(lp);
				}
				else {
					tmpLocal = new ImageView(this);
					tmpLocal.setImageResource(R.drawable.local_camera);
					LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
					mLocalViewLayout.addView(tmpLocal,lp);
				} /* mLocalView != null */
				//}
			} /* ScreenSize >= 4 */
		} /* isScreenPortrait == false */
	}

	private void initRemoteView()
	{
		if (isScreenPortrait == true) {
			if((Hack.isK72()|| Hack.isQF3()) && QtalkSettings.enable1832) {
				RemoteViewH = screenH;
				RemoteViewW = RemoteViewH*16/9;
			}
			else if(Hack.isQF3a()||Hack.isQF5()){
				RemoteViewH = screenH;
				RemoteViewW = RemoteViewH*16/9;
			}
			else{
				if(initRemoteH == 0 || initRemoteW==0){
					RemoteViewH = initRemoteH;
					RemoteViewW = initRemoteW;
				}else{
					RemoteViewW = screenW;
					RemoteViewH = (int)((float)screenW*remoteRatio);
				}
			}

			mRemoteViewLayout = (RelativeLayout)mLayout.findViewById(R.id.remote_view_ll);
			if(mRemoteView!=null) {
				LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(RemoteViewW, RemoteViewH);
				mRemoteViewLayout.addView(mRemoteView,lp);//   mVideoRender
			}
			else {
				mRemoteViewLayout.setBackgroundColor(Color.TRANSPARENT);//DKGRAY
			}
		}
		else {
			if(ScreenSize < 4){
				if(initRemoteH == 0 || initRemoteW == 0) {
					RemoteViewH = initRemoteH;
					RemoteViewW = initRemoteW;
				}
				else {
					RemoteViewH = screenH;
					RemoteViewW = (int)((float)screenH/remoteRatio);
				}

				mRemoteViewLayout = (RelativeLayout)mLayout.findViewById(R.id.remote_view_ll);
				if(mRemoteView!=null) {
					LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(RemoteViewW, RemoteViewH);
					mRemoteViewLayout.addView(mRemoteView,lp);
				}
				else {
					mRemoteViewLayout.setBackgroundColor(Color.TRANSPARENT);//DKGRAY
				}
			}
			else {   // screen size 4, use another UI----------------------------------------------------
				RemoteViewH = 0;
				RemoteViewW = 0;
				mRemoteViewLayout = (RelativeLayout)mLayout.findViewById(R.id.remote_view_ll);
				if(mRemoteView!=null) {
					LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(RemoteViewW, RemoteViewH);
					mRemoteViewLayout.addView(mRemoteView,lp);//   mVideoRender
				}
				else {
					mRemoteViewLayout.setBackgroundColor(Color.TRANSPARENT);//DKGRAY
				}
			}
		}
		mRemoteViewLayout.addView(holdimg,new LinearLayout.LayoutParams(RemoteViewW, RemoteViewH));
	}


	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@SuppressLint({ "NewApi", "ClickableViewAccessibility" })
	public void initGUI()
	{
		computeScreenSize();

		if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
			setContentView(R.layout.layout_invideosession_land_k72);
		}
		else if(Hack.isQF3a()||Hack.isQF5()||Hack.isStation()||Hack.isPT()) {
			setContentView(R.layout.layout_invideosession_land_k72);
		}
		else {
			if(isScreenPortrait) setContentView(R.layout.layout_invideosession_port);
			else setContentView(R.layout.layout_invideosession_land);
		}

		if (mLocalViewLayout!=null) mLocalViewLayout.removeAllViews();
		else mLocalViewLayout = new RelativeLayout(this);
		mLocalViewLayout.setGravity(Gravity.CENTER);

		if (mRemoteViewLayout!=null) mRemoteViewLayout.removeAllViews();
		else mRemoteViewLayout = new RelativeLayout(this);
		mRemoteViewLayout.setGravity(Gravity.CENTER_VERTICAL);

		if (mCameraViewLayout!=null) mCameraViewLayout.removeAllViews();
		else mCameraViewLayout = new RelativeLayout(this);
		mCameraViewLayout.setGravity(Gravity.TOP);

		if (mLayout!=null) mLayout.removeAllViews();
		else mLayout=new RelativeLayout(this);

		if(mVideosessionbarContainer!=null ) mVideosessionbarContainer.removeAllViews();
		else mVideosessionbarContainer=new LinearLayout(this);  //  Relative

		if(endcallContainer!=null ) endcallContainer.removeAllViews();
		else endcallContainer=new RelativeLayout(this);

		if(mTitleBarLayout != null)	mTitleBarLayout.removeAllViews();
		else mTitleBarLayout=new RelativeLayout(this);

		if(mLowbandwidth!=null) mLowbandwidth.removeAllViews();
		else mLowbandwidth = new RelativeLayout(this);

		if(mHoldLayout!=null) mHoldLayout.removeAllViews();
		else mHoldLayout = new RelativeLayout(this);

		Configuration config = getResources().getConfiguration();
		int navigationbar_heightid = getResources().getIdentifier("navigation_bar_height", "dimen", "android");
		int navigationbar_widthid = getResources().getIdentifier("navigation_bar_width", "dimen", "android");

		if(navigationbar_heightid > 0){
			navigationbar_height = getResources().getDimensionPixelSize(navigationbar_heightid);
		}
		if(navigationbar_widthid > 0){
			navigationbar_width = getResources().getDimensionPixelSize(navigationbar_widthid);
		}
		ScreenSize = (config.screenLayout&Configuration.SCREENLAYOUT_SIZE_MASK);

		//==The base layout================================
		mLayout = (RelativeLayout)findViewById(R.id.videosession_bg);
		if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) >= 16) {
			mLayout.setBackground(getResources().getDrawable(R.drawable.bg_video_session));
		}else{
			mLayout.setBackgroundResource(R.drawable.bg_video_session);
		}

		mInitVideoConnect = (ImageView)findViewById(R.id.init_connecting);
		mConnectAnimation = (AnimationDrawable)mInitVideoConnect.getBackground();
		if(mConnectAnimation == null){
			Log.d(DEBUGTAG,"mConnectAnimation = null");
		}else{
			Log.d(DEBUGTAG,"mConnectAnimation"+mConnectAnimation);
		}
		mInitVideoConnect.post(init_video_animation);

		mLowbandwidth = (RelativeLayout)findViewById(R.id.LowBandwidth);
		mLowbandTextView = (TextView)findViewById(R.id.LowBandwidth_tv);
		mHoldLayout = (RelativeLayout) findViewById(R.id.Hold_rl);
		mControlLayout = (RelativeLayout)findViewById(R.id.control_rl);
		mControlLayout.bringToFront();

		mTimerTextView = new TextView(this);
		if (Name == null) {
			String callerIDContent = mRemoteID;
			if(null == mRemoteID){
				callerIDContent = "LocalLoopBack";
			}
			QtalkDB qtalkdb = QtalkDB.getInstance(this);
			
			Cursor cs = qtalkdb.returnAllPBEntry();
			int size = cs.getCount();
			cs.moveToFirst();
			int cscnt = 0;
			for( cscnt = 0; cscnt < cs.getCount(); cscnt++)
			{
				PhoneNumber = QtalkDB.getString(cs, "number");
				if (PhoneNumber.contentEquals(callerIDContent))
				{
					Name = QtalkDB.getString(cs, "name");
					Role = QtalkDB.getString(cs, "role");
				}
				cs.moveToNext();
			}
			if(Name==null){
				Name=callerIDContent;
			}

			if(Name!=null){
				String[] name_split = Name.split(":");
				String name_phrase = null ;
				if(name_split.length>1){
					Name = name_split[1];
				}else{
					Name = name_split[0];
				}
			}
			cs.close();
			qtalkdb.close();
		}

		if (ScreenSize < 4) { // For 7" Screen Size
			TextView mName = new TextView(this);
			mName.setText(Name);
			mName.setTextSize(25);
			mName.setTextColor(android.graphics.Color.rgb(51, 51, 51));
			mName.setTypeface(Typeface.DEFAULT_BOLD);
			mName.setGravity(Gravity.CENTER);
		}
		else { // For 4 Screen Size
			TextView mName = new TextView(this);
			mName.setText(Name);
			mName.setTextSize(45);
			mName.setTextColor(android.graphics.Color.rgb(51, 51, 51));
			mName.setTypeface(Typeface.DEFAULT_BOLD);
			mName.setGravity(Gravity.CENTER);
			RelativeLayout.LayoutParams nbp = new RelativeLayout.LayoutParams(
					ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.MATCH_PARENT);
		}

		initButtonListener();

		initLocalView();

		initRemoteView();

		if(isholdnow){
			holdimg.setVisibility(View.VISIBLE);
			holdimg.bringToFront();
			mHoldLayout.setVisibility(View.VISIBLE);
		}
		else {
			holdimg.setVisibility(View.INVISIBLE);
			mHoldLayout.setVisibility(View.INVISIBLE);
		}

		Log.d(DEBUGTAG, "initGUI(), screen:"+screenW+"x"+screenH+
				", localview:"+LocalViewW+"x"+LocalViewH+
				", remoteview:"+RemoteViewW+"x"+RemoteViewH+
				", ScreenSize:"+ScreenSize+
				", isScreenPortrait:"+isScreenPortrait);

	} //end of initGUI

	@SuppressLint("InlinedApi")
	private void setSessionTimer() {
		mTimerTextView = (TextView)mLayout.findViewById(R.id.mTimerTextView);
		mTimerTextView.setPadding(0, screenH/20, 0, 0);
	}

	String ntohl(int i)
	{
		int i5 = i & 0xFF;
		int i6 = (i >> 8) & 0xFF;
		int i7 = (i >> 16) & 0xFF;
		int i8 = (i >> 24) & 0xFF;
		return ""+i5+"."+i6+"."+i7+"."+i8;
	}
	protected void onStart()
	{
		super.onStart();
	}


	public void onStop(){
		super.onStop();
		DisableAllCounter();
		myOrientationEventListener.disable();
		releaseAll();
		finishAffinity();
	}

	// ROTATE =============================================================================
	@SuppressWarnings("deprecation")
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		float touchX = event.getX();       // 觸控的 X 軸位置
		float touchY = event.getY();  // 觸控的 Y 軸位置
		int lengthX = mLocalViewLayout.getWidth()/2;
		int lengthY = mLocalViewLayout.getHeight()/2;
		//int diff_startW = (screenW)/20;
		//int diff_startH = (screenH)*3/5;
		// 判斷觸控動作
		switch( event.getAction() ) {

			case MotionEvent.ACTION_DOWN:  // 按下
				if(localview_moving){
					//Log.d("moving","lengthX: "+lengthX+" lengthY: "+lengthY);
					//Log.d("moving","action down touchX:"+touchX+" , touchY:"+touchY+" view.getX: "+mLocalViewLayout.getX()+" view.getY: "+mLocalViewLayout.getY());
				}
				break;

			case MotionEvent.ACTION_MOVE:  // 拖曳移動
				if(localview_moving){
					Log.d(DEBUGTAG,"moving touchX: "+touchX+" touchY: "+touchY +" lengthX:"+lengthX+" lengthY:"+lengthY
							+" LocalViewStartX:"+LocalViewStartX+" LocalViewStartY:"+LocalViewStartY);

					float setX = touchX-lengthX-LocalViewStartX;
					float setY = touchY-lengthY-LocalViewStartY;
					if(touchX-lengthX-LocalViewStartX<(-1)*LocalViewStartX){
						setX = (-1)*LocalViewStartX;
					}else if(touchX+lengthX>screenW){
						setX = screenW-(lengthX*2)-LocalViewStartX;
					}
					if(touchY-lengthY-LocalViewStartY<(-1)*LocalViewStartY){
						setY = (-1)*LocalViewStartY;
					}else if(touchY+lengthY>screenH){
						setY = screenH-(lengthY*2)-LocalViewStartY;
					}
					//setX = setX-diff_startW;

					mLocalViewLayout.setTranslationX(setX);
					mLocalViewLayout.setTranslationY(setY);
					//Log.d("moving","moving setX: "+setX+" setY: "+setY+" view.getX: "+mLocalViewLayout.getX()+" view.getY: "+mLocalViewLayout.getY());
				}
				break;

			case MotionEvent.ACTION_UP:  // 放開
				mEndCall_rl.bringToFront();
				mEndCallBtn.bringToFront();
				localview_moving = false;
				break;
		}

		return super.onTouchEvent(event);
	}
	@SuppressLint("InlinedApi")
	private void HideNavigationBar(int sec) {
		if(mHandler!=null)
			mHandler.postDelayed(hide_navigation, sec);
	}
	private void ShowNavigationBar(int sec) {
		if(mHandler!=null)
			mHandler.postDelayed(show_navigation, sec);
	}
	private void setButtons(){
		//INIT MENU BUTTONS/////////========================================================
		// if(mEndCallBtn==null)
		mEndCallBtn=(Button)mVideosessionbar.findViewById(R.id.mEndCallBtn);
		// if(mTimerTextView==null)
		mTimerTextView = (TextView) mVideosessionbar.findViewById(R.id.mTimerTextView);

		mEndCallBtn.setOnTouchListener(
				new OnTouchListener(){
					@Override
					public boolean onTouch(View v, MotionEvent event) {
						int Eventaction = event.getAction();
						if(ViroActivityManager.isCurrentPageTopActivity(InVideoSessionActivity.this,InVideoSessionActivity.class.getCanonicalName()))
						{
							switch(Eventaction)
							{
								case MotionEvent.ACTION_DOWN:
									//mEndCallBtn.setImageResource(R.drawable.ic_end_new);
									//Toast.makeText(InVideoSessionActivity.this,"View="+v.getId(),Toast.LENGTH_SHORT).show();

									Intent EndPopwindow = new Intent();
									EndPopwindow.setClass(InVideoSessionActivity.this, VideoEndPopWindow.class);
									startActivity(EndPopwindow);
									finish();
									break;
								case MotionEvent.ACTION_UP:
									//
									break;
							}
						}
						return false;
					}
				});//end of mEndCallBtn setOnTouchListener
	} //END of setButtons

	//Backend related functions===============================================================================
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
		if(localViewDrag==false){
			mTimerTextView.setText(str_minutes+":"+str_seconds);
		}
	}
	@Override
	public boolean dispatchTouchEvent(MotionEvent ev) {
		int action = ev.getAction();
		if(action == MotionEvent.ACTION_DOWN){
			if(isShowBar){
				if(mHandler!=null)
					mHandler.postDelayed(hide_navigation, 0);
			}
			else{
				if(mHandler!=null)
					mHandler.postDelayed(show_navigation, 0);
			}
		}
		return super.dispatchTouchEvent(ev);
	}
	@Override
	protected void setLocalViewResolution(int width, int height)
	{
		// mLocalViewLayout.setResolution(width,height);
	}

	@Override
	protected void onCameraModeChanged(final boolean isDualCamera,final boolean isShareMode)
	{

	}
	@Override
	public void onSelfCallChange(boolean enableVideo)//Self do re-invite
	{

	}
	@Override
	public void onRemoteCallChange(final boolean enableVideo)//Remote do re-invite
	{

	}

	@Override
	public void onReinviteFailed()
	{

	}
	@Override
	public void onRemoteHangup(String message)
	{
		Log.d(DEBUGTAG,"onRemoteHangup:"+message);
		RingingUtil.playSoundHangup(this);
		skipOnPause = false;
		isRemoteHangup =true;
/*    	Intent EndPopwindowR = new Intent();
		EndPopwindowR.setClass(InVideoSessionActivity.this, VideoEndPopWindow.class);
		startActivity(EndPopwindowR);*/
		AppStatus.setStatus(AppStatus.IDEL);
		finish();
	}
	@Override
	public void onRemoteCallHold(boolean isHeld)
	{
		Log.d(DEBUGTAG,"==>onRemoteCallHold :"+isHeld);
		isholdnow = isHeld;
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				// TODO Auto-generated method stub
				if(isholdnow){
					holdimg.setVisibility(View.VISIBLE);
					holdimg.bringToFront();
					mHoldLayout.setVisibility(View.VISIBLE);
				}else{
					holdimg.setVisibility(View.INVISIBLE);
					mHoldLayout.setVisibility(View.INVISIBLE);
				}
			}
		});
		setHold(isHeld);
	}
	public void removeVideoSessionView()
	{
		if(mRemoteViewLayout != null)
			mRemoteViewLayout.removeAllViews();
		if(mLocalViewLayout != null)
			mLocalViewLayout.removeAllViews();
		if(mCameraViewLayout != null)
			mCameraViewLayout.removeAllViews();
	}
	private double last_pkloss = 0;
	@Override
	public void onReport(final int kbpsRecv,final byte fpsRecv,final int kbpsSend,
						 final byte fpsSend,final double pkloss,final int usRtt)
	{
		//Log.d(DEBUGTAG,"onReport:recv:("+kbpsRecv+"kbps,"+fpsRecv+"fps)"+"send:("+kbpsSend+"kbps,"+fpsSend+"fps)"+"pkloss:"+pkloss+",rtt:"+usRtt);
		final double local_pkloss = pkloss*100;
		//Log.d(DEBUGTAG, "set alpha:"+poorNetwork+":"+(int)(Math.log10(local_pkloss))*100);
		if(last_pkloss != local_pkloss)
		{
			last_pkloss = local_pkloss;
			if (mHandler != null) {
				mHandler.post(new Runnable(){
					@Override
					public void run()
					{
					/*	if (poorNetwork != null)
							poorNetwork.setAlpha((int)(Math.log10(local_pkloss))*100);*/
					}
				});
			}
		}
	}
	@Override
	public boolean onDown(MotionEvent e)
	{
		// TODO Auto-generated method stub
		return false;
	}
	@Override
	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
						   float velocityY)
	{
		// TODO Auto-generated method stub
		return false;
	}
	@Override
	public void onLongPress(MotionEvent e)
	{
		// TODO Auto-generated method stub

	}
	@Override
	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
							float distanceY)
	{
		// TODO Auto-generated method stub
		return false;
	}
	@Override
	public void onShowPress(MotionEvent e)
	{
		// TODO Auto-generated method stub

	}
	@Override
	public boolean onSingleTapUp(MotionEvent e)
	{
		// TODO Auto-generated method stub
		return false;
	}
	public static int getNavigationBarHeight(Context context) {
		Resources resources = context.getResources();
		int resourceId = resources.getIdentifier("navigation_bar_height", "dimen", "android");
		if (resourceId > 0) {
			return resources.getDimensionPixelSize(resourceId);
		}
		return 0;
	}
	private void releaseAll()
	{
		removeVideoSessionView();
		unregisterReceiver(session_event_receiver);
		if(muteBmp!=null){
			muteBmp.recycle();
			muteBmp = null;
		}
		if(reSizeMuteBmp!=null){
			reSizeMuteBmp.recycle();
			reSizeMuteBmp = null;
		}
		mHandler=null;
		mLayout=null;

		mTimerTextView = null;
		mCameraView = null;
		//       mLocalView = null;
		mRemoteView = null;
		mCameraViewLayout = null;
//        mCameraBGLayout = null;
//        mRemoteBGLayout = null;
		mTitleBarLayout = null;
		mLocalViewLayout = null;
		mRemoteViewLayout = null;
		if(mConnectAnimation!=null){
			mConnectAnimation.stop();
			mConnectAnimation = null;
		}
		if(mInitVideoConnect!=null){
			mInitVideoConnect.removeCallbacks(init_video_animation);
			mInitVideoConnect = null;
		}
		mVideosessionbar = null;
		mVideosessionbarContainer = null;
		mLowbandwidth = null;
		endcallContainer = null;
		swapP=null;
		swapPFish=null;
		endlp=null;
		mShareBtn = null;
		mEndCallBtn=null;
		mToAudioBtn=null;
 /*       mMuteBtn=null;
		mDTMFBtn=null;*/
		tmpLocal=null;
		mLocalBG=null;
		tmpRemote=null;
		vIcon = null;
		//       poorNetwork=null;
		li=null;
		Name = null;
		PhoneNumber = null;
//        Image = null;
		sr2_switchcountdown_tv = null;
		sr2_switchpx_list = null;
		timer = null;
		task = null;
		hTimer = null;
		hTask = null;
		mData = null;
		decorView = null;
	}

}
