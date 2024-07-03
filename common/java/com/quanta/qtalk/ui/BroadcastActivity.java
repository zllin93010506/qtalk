package com.quanta.qtalk.ui;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.ui.contact.QTDepartment;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils.TruncateAt;
import android.util.DisplayMetrics;
import com.quanta.qtalk.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class BroadcastActivity extends Activity {
	private static final String DEBUGTAG = "QTFa_BroadcastActivity";
	private String provisionID = null;
	private String provisionType = null;
	private String broadcast_uri = null;
	private String room_number = null;
	private static Context mContext = null;
	private List<String> ttslist;
	private ArrayList<QTDepartment> deplist = new ArrayList<QTDepartment>();
	private String ttsmessage = null;
	RelativeLayout tts_check_xlarge;
	private static final int MCU_SUCCESS = 9;
	private static final int MCU_FAIL = 10;
	private static final int MCU_BUSY = 11;
	private static final int MCU_IN_PROGRESS = 12;
	private int ScreenSize = 0, screenwidth = 0, screenheight = 0;
	
	
	AlertDialog connect_popout = null, 
				text_popout = null, 
				list_popout = null, 
				ptt_popout = null, 
				tts_popout = null,
				choose_popout = null;
	Button ptt_btn = null;
	ScrollView tts_scl = null;
	private ArrayList<String> depID = new ArrayList<String>();
	
	private BroadcastReceiver receiver = new BroadcastReceiver(){

		@Override
		public void onReceive(Context context, Intent intent) {
			if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
				//finish();
				Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
			}else if(intent.getAction().equals(LauncherReceiver.ACTION_VC_TTS_CB)){
				mHandler.removeCallbacks(ttsCBtimeoutThread);
				Bundle bundle = intent.getExtras();
				String msg = bundle.getString("KEY_STATE");
				if(msg!=null){
					Log.d(DEBUGTAG,"receive tts cb:"+msg);
					if(msg.contentEquals("SUCCESS")){
				        mHandler.postDelayed(ttsSuccessThread, 1000);
				        mHandler.postDelayed(finishThread, 2000);
					}else{
						mHandler.postDelayed(ttsFailThread, 1000);
				        mHandler.postDelayed(finishThread, 2000);
					}
				}
			}
		}
    };

	private final Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MCU_SUCCESS:
				String MCU = msg.getData().getString("room_number");
				Log.d(DEBUGTAG, "MCU number" + MCU);
				if (MCU != null) {
					try {
						QTReceiver.engine(mContext, false).makeCall(null, MCU,
								false,false);
					} catch (FailedOperateException e) {
						Log.e(DEBUGTAG, "makeCall", e);
					}
				}
				mHandler.postDelayed(finishThread, 3000);
				break;
			case MCU_FAIL:
				Log.d(DEBUGTAG,"broadcast mcu fail!!!!!!");
	    		openTextDialog(getString(R.string.fail_send_ptt));
				mHandler.postDelayed(finishThread, 2000);
				connect_popout.dismiss();
			case MCU_BUSY:
				Log.d(DEBUGTAG,"broadcast mcu fail!!!!!!");
	    		openTextDialog(getString(R.string.no_available_broadcast_agent));
				mHandler.postDelayed(finishThread, 2000);
				connect_popout.dismiss();
			case MCU_IN_PROGRESS:
				Log.d(DEBUGTAG,"broadcast mcu fail!!!!!!");
	    		openTextDialog(getString(R.string.department_during_broadcasting));
				mHandler.postDelayed(finishThread, 2000);
				connect_popout.dismiss();				
				break;
			}
		}
	};
	
	private RadioGroup.OnCheckedChangeListener mTtsChangeRadio = new  RadioGroup.OnCheckedChangeListener(){

		@Override
		public void onCheckedChanged(RadioGroup group, int checkedId) {
			if(!Hack.isStation()){
				openTTScheckDialog(checkedId);
				list_popout.dismiss();
			}else{
				tts_check_xlarge.setVisibility(View.VISIBLE);
				ttsmessage = ttslist.get(checkedId).toString();
				ptt_btn.setVisibility(View.INVISIBLE);
				//RelativeLayout broadcast_bg = (RelativeLayout)findViewById(R.id.broadcast_bg);
				//broadcast_bg.setBackgroundResource(R.drawable.img_popout_broadcast_tablet_2);
			}
		}
		
	};
	private RadioGroup.OnCheckedChangeListener mDepChangeRadio = new  RadioGroup.OnCheckedChangeListener(){

		@Override
		public void onCheckedChanged(RadioGroup group, int checkedId) {
			if(!Hack.isStation()){
				depID.clear();
				depID.add(deplist.get(checkedId).getDepID());
				openBroadcastListDialog();
				choose_popout.dismiss();
			}else{
				//tts_check_xlarge.setVisibility(View.VISIBLE);
				//ttsmessage = ttslist.get(checkedId).toString();
				//ptt_btn.setVisibility(View.INVISIBLE);
			}
		}
		
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(DEBUGTAG,"OnCreate");
		mContext = this;
		if(QtalkSettings.ScreenSize<0){
			Hack.getScreenSize(this);
		}
		ttslist = new ArrayList<String>();
		setContentView(R.layout.layout_broadcast);
		Intent intent = new Intent();
        intent.setAction(LauncherReceiver.ACTION_GET_TTS);
        sendBroadcast(intent);
		String path = Hack.getStoragePath() + "ttsList";
		try {
			BufferedReader br = new BufferedReader(new FileReader(path));
			while (br.ready()) {
				String item = br.readLine();
				//Log.d(DEBUGTAG, item);
				ttslist.add(item);
			}
			br.close();
		} catch (Exception e) {
			Log.e(DEBUGTAG,"",e);
		} 
		
		try {
			deplist = ProvisionUtility.parseProvisionDepartment(Hack.getStoragePath()+"provision.xml");
		} catch (ParserConfigurationException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"",e);
		} catch (SAXException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"",e);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"",e);
		}

		ScreenSize = QtalkSettings.ScreenSize;
        screenwidth = QtalkSettings.screenwidth;
        screenheight = QtalkSettings.screenheight;
	}
	private void sendttsmessage() {
		if(ttsmessage!=null){
			if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"ttsmessage="+ttsmessage);
			if(Hack.isStation()){
				tts_check_xlarge.setVisibility(View.INVISIBLE);
			}
			IntentFilter intentfilter = new IntentFilter();
    		intentfilter.addAction(LauncherReceiver.ACTION_VC_TTS_CB);
            registerReceiver(receiver, intentfilter); 
			openTextDialog(getString(R.string.tts_sending));

			Intent intent = new Intent();
	        intent.setAction(LauncherReceiver.ACTION_VC_TTS);
	        Bundle bundle = new Bundle();
	        bundle.putString("KEY_TTS_MSG" , ttsmessage);
	        bundle.putStringArrayList("KEY_NS_ID", depID);
	        intent.putExtras(bundle);
	        sendBroadcast(intent);
            mHandler.postDelayed(ttsCBtimeoutThread, 5000);

		}
	}
	protected  void finishActivity() {
		finish();
	}

	@Override
	protected void onStart() {
		Log.d(DEBUGTAG,"OnStart");
		super.onStart();
		if(deplist!=null && deplist.size()>1){
			openChooseDepDialog();
		}
		else{
			if(deplist!=null && deplist.size()==1)
				depID.add(deplist.get(0).getDepID());
			openBroadcastListDialog();
		}

	}

	@Override
	protected void onResume() {
		Log.d(DEBUGTAG,"OnResume");	
		super.onResume();
	}
	@Override
	protected void onDestroy() {
		releaseAll();
		super.onDestroy();
	}
	private void releaseAll(){
		if(connect_popout!=null){
			connect_popout.dismiss();
			connect_popout = null;
		}
		if(text_popout!=null){
			text_popout.dismiss();
			text_popout = null;
		}
		if(list_popout!=null){
			list_popout.dismiss();
			list_popout = null;
		}
		if(ptt_popout!=null){
			ptt_popout.dismiss();
			ptt_popout = null;
		}
		if(tts_popout!=null){
			tts_popout.dismiss();
			tts_popout = null;
		}
		if(choose_popout!=null){
			choose_popout.dismiss();
			choose_popout = null;
		}
	
	}
	Runnable sendMCU = new Runnable() {
		@Override
		public void run() {
			sendMCURequest();
		}
	};
	private void sendMCURequest() {
		Log.d(DEBUGTAG,"sendMCURequest");
		try {
			final QtalkSettings qtalk_settings = QTReceiver.engine(this, false)
					.getSettings(this);
			provisionID = qtalk_settings.provisionID;
			provisionType = qtalk_settings.provisionType;
		} catch (FailedOperateException e1) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"",e1);
		}
		//[temp] only one dep will be ppt
		String dep = null;
		if(depID!=null && depID.size()>0){
			dep = depID.get(0);
			Log.d(DEBUGTAG,"dep:"+depID.get(0));
		}

		if (QThProvisionUtility.provision_info != null) {
			broadcast_uri = QThProvisionUtility.provision_info.phonebook_uri
					+ provisionType + "&service=broadcastmcu&uid="
					+ provisionID + "&action=get_room_number&dep_id="+dep;
			try {
				QThProvisionUtility qtnMessenger = new QThProvisionUtility(this, mHandler,
						null);
				qtnMessenger.httpsConnect(broadcast_uri, room_number);
			} catch (Exception e) {
				Log.e(DEBUGTAG, "BroadcastMCU", e);
				sendPTTwithoutNetwork();
				Log.e(DEBUGTAG,"",e);
			}
		} else {
			String provision_file = Hack.getStoragePath()
					+ "provision.xml";
			try {
				QThProvisionUtility.provision_info = ProvisionUtility
						.parseProvisionConfig(provision_file);
			} catch (ParserConfigurationException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			} catch (SAXException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			}
		}

	}
    private void sendPTTwithoutNetwork() {
		mHandler.postDelayed(finishThread, 1000);
		 Intent intent = new Intent(mContext,OutgoingCallFailActivity.class);
		Bundle bundle = new Bundle();
        bundle.putInt("KEY_ERRORCODE"  , 1);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.putExtras(bundle);
		mContext.startActivity(intent); 
	}
	final Runnable finishThread = new Runnable() {
        public void run() {
            finish();
        }
    };
    final Runnable ttsCBtimeoutThread = new Runnable(){
    	public void run(){
    		Log.d(DEBUGTAG,"ttsCB timeout");
    		unregisterReceiver(receiver);
    		openTextDialog(getString(R.string.fail_send_tts));
        	mHandler.postDelayed(finishThread, 2000);
    	}
    };
    final Runnable ttsSuccessThread = new Runnable() {
        public void run() {
        	openTextDialog(getString(R.string.success_send_tts));
        	unregisterReceiver(receiver);
        }
    };
    final Runnable ttsFailThread = new Runnable() {
        public void run() {
        	openTextDialog(getString(R.string.fail_send_tts));
        	unregisterReceiver(receiver);;
        }
    };

	void openTTScheckDialog(final int checkedId) {
		if(tts_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			tts_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(true).create();
		}
		tts_popout.show();

		Window win = tts_popout.getWindow();
		win.setContentView(R.layout.broadcast_tts_dialogue);
		ttsmessage = ttslist.get(checkedId).toString();
		TextView text_tts_check = (TextView) win.findViewById(R.id.text_tts_check);
		text_tts_check.setText(ttslist.get(checkedId).toString());
		text_tts_check.setTextAppearance(mContext, R.style.BroadcastPtt);
		Button btn_tts_cancel = (Button) win.findViewById(R.id.btn_close);
		btn_tts_cancel.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				finish();
			}
		});
		Button btn_tts_comfirm = (Button) win.findViewById(R.id.btn_tts_confirm);
		btn_tts_comfirm.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				sendttsmessage();
				tts_popout.dismiss();
			}
		});

		OnCancelListener cancellistener = new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				finish();
			}
		};
		
	    tts_popout.setOnCancelListener(cancellistener);
		
	}
	void openPTTcheckDialog(){
		if(ptt_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			ptt_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(true).create();
		}
		ptt_popout.show();

		Window win = ptt_popout.getWindow();
		win.setContentView(R.layout.broadcast_ptt_dialogue);
		Button btn_ptt_cancel = (Button) win.findViewById(R.id.btn_close);
		btn_ptt_cancel.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				finish();
			}
		});
		Button btn_ptt_comfirm = (Button) win.findViewById(R.id.btn_ptt_confirm);
		btn_ptt_comfirm.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Log.d(DEBUGTAG,"onClick sendMCURequest");
				new Thread(sendMCU).start();
				openPTTconnectDialog();
				ptt_popout.dismiss();
			}
		});

		OnCancelListener cancellistener = new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				finish();
			}
		};
		
	    ptt_popout.setOnCancelListener(cancellistener);
	}
	void openPTTconnectDialog(){
		if(connect_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			connect_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(false).create();
		}
		connect_popout.show();

		Window win = connect_popout.getWindow();
		win.setContentView(R.layout.broadcast_ptt_connect_dialogue);

	}
	void openTextDialog(String msg){
		if(text_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			text_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(false).create();
		}
		text_popout.show();

		Window win = text_popout.getWindow();
		win.setContentView(R.layout.broadcast_text_dialogue);
		TextView tv = (TextView)win.findViewById(R.id.text_content);
		tv.setText(msg);

	}
	void openChooseDepDialog(){
		if(choose_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			choose_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(true).create();	
		}
		choose_popout.show();
		Window win = choose_popout.getWindow();
		win.setContentView(R.layout.broadcast_choose_dep_dialogue);
		Button btn_close = (Button)win.findViewById(R.id.btn_close);
		final RadioGroup depRadioGroup = (RadioGroup)win.findViewById(R.id.depRadioGroup);
		btn_close.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				finish();
			}
		});
		if (deplist != null) {
			if(depRadioGroup.getChildCount()==0){
				for(int i =0; i<deplist.size();i++){
	
					RadioButton chkBshow = new RadioButton(this);
					
					chkBshow.setText(deplist.get(i).getDepName());
					chkBshow.setTextAppearance(mContext, R.style.ttsList);
					chkBshow.setSingleLine(true);
					chkBshow.setButtonDrawable(R.drawable.trans1x1);
					chkBshow.setBackgroundResource(R.drawable.rdbcheck_background);
					chkBshow.setId(i);
					if(ScreenSize==4){
						chkBshow.setPadding(40, 0, 0, 0);
					}else{
						chkBshow.setPadding(80, 0, 0, 0);
					}
					depRadioGroup.addView(chkBshow);
				}

				depRadioGroup.setOnCheckedChangeListener(mDepChangeRadio);
			}
		}
		
		OnCancelListener cancellistener = new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				finish();
			}
		};
		
	    choose_popout.setOnCancelListener(cancellistener);
		
		
	}
	void openBroadcastListDialog(){
		if(list_popout==null){
			AlertDialog.Builder builder = new AlertDialog.Builder(
					BroadcastActivity.this);
			list_popout =  builder.setTitle(R.string.app_name).setMessage("waiting")
					.setCancelable(true).create();	
		}
		list_popout.show();
		
		Window win = list_popout.getWindow();
		win.setContentView(R.layout.broadcast_list_dialogue);

		ptt_btn = (Button) win.findViewById(R.id.ptt_btn);
		tts_scl = (ScrollView) win.findViewById(R.id.tts_ll);
		ptt_btn.setPadding(screenwidth/38, 0, 0, 0);
		final RadioGroup ttsRadioGroup = (RadioGroup)win.findViewById(R.id.ttsRadioGroup);
		TextView empty_message = (TextView)win.findViewById(R.id.empty_message);
		Button btn_close = (Button)win.findViewById(R.id.btn_close);
		if(ScreenSize==4){
			list_popout.getWindow().setLayout(screenwidth/2,screenheight*3/5);
			empty_message.getLayoutParams().width = (int)((double)(screenwidth)/4.8);
			tts_scl.getLayoutParams().height = (int)((double)(screenheight)/2.3);
			tts_scl.setPadding(0, screenheight/50, 0, 0);
			RelativeLayout.LayoutParams ptt_btn_para = (RelativeLayout.LayoutParams) ptt_btn.getLayoutParams();
			if(screenwidth>1900){
				ptt_btn_para.topMargin = screenheight / 26;
			}else{
				ptt_btn_para.topMargin = screenheight / 40;
			}
			
		}
		
		ptt_btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				openPTTcheckDialog();
				list_popout.dismiss();
			}

		});
		
		if (ttslist != null) {
			if(ttsRadioGroup.getChildCount()==0){
				for(int i =0; i<ttslist.size();i++){
	
					RadioButton chkBshow = new RadioButton(this);
					
					chkBshow.setText(ttslist.get(i).toString());
					//chkBshow.setMarqueeRepeatLimit(6);
					chkBshow.setEllipsize(TruncateAt.END);
					chkBshow.setTextAppearance(mContext, R.style.ttsList);
					chkBshow.setSingleLine(true);
					//chkBshow.setSelected(true);
					chkBshow.setButtonDrawable(R.drawable.trans1x1);
					chkBshow.setBackgroundResource(R.drawable.rdbcheck_background);
					chkBshow.setId(i);
					if(ScreenSize==4){
						chkBshow.setPadding(40, 0, 100, 0);
						chkBshow.setMaxWidth(300);
						chkBshow.setSelected(true);
						chkBshow.setMarqueeRepeatLimit(6);
						chkBshow.setEllipsize(TruncateAt.MARQUEE);
					}else{
						chkBshow.setPadding(50, 0, 150, 0);
					}
					ttsRadioGroup.addView(chkBshow);
				}
				
				ttsRadioGroup.setOnCheckedChangeListener(mTtsChangeRadio);
			}
		if(ttslist.size()==0){
			empty_message.setVisibility(View.VISIBLE);
		}else{
			empty_message.setVisibility(View.GONE);
		}
			
		}
		btn_close.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		if(!Hack.isStation()){
			
		}else{
			tts_check_xlarge = (RelativeLayout)win.findViewById(R.id.rl_tts_xlarge_check);
			RelativeLayout.LayoutParams tts_check_xlarge_para = (RelativeLayout.LayoutParams) tts_check_xlarge.getLayoutParams();
			if(screenwidth>=1600){
				tts_check_xlarge_para.topMargin = screenheight / 22;
			}else{
				tts_check_xlarge_para.topMargin = screenheight / 40;
			}
			Button btn_tts_cancel_xlarge = (Button)win.findViewById(R.id.btn_tts_cancel_xlarge);
			btn_tts_cancel_xlarge.setOnClickListener(new OnClickListener() {
				
				@Override
				public void onClick(View v) {
					finish();
				}
			});		
			Button btn_tts_comfirm_xlarge = (Button)win.findViewById(R.id.btn_tts_confirm_xlarge);
			btn_tts_comfirm_xlarge.setOnClickListener(new OnClickListener() {
				
				@Override
				public void onClick(View v) {
					sendttsmessage();
					
				}
			});
			
			tts_check_xlarge.setVisibility(View.INVISIBLE);
		}
		
		OnCancelListener cancellistener = new OnCancelListener() {
			
			@Override
			public void onCancel(DialogInterface dialog) {
				finish();
			}
		};
		
	    list_popout.setOnCancelListener(cancellistener);
	}
	
}