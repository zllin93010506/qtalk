package com.quanta.qtalk.ui;

import java.io.File;

import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import com.quanta.qtalk.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

public class OutgoingCallFailActivity extends AbstractVolumeActivity
{
	Handler mHandler=new Handler();
	TextView informationText;
	Button btn_close;
	private static final String DEBUGTAG = "QTFa_OutgoingCallFailActivity";
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		Log.d(DEBUGTAG,"==> onCreate");
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
		if(QtalkSettings.nurseCall){
			QtalkSettings.nurseCall = false;
		}
		setContentView(com.quanta.qtalk.R.layout.layout_outgoingcallfail);
		Bundle bundle = this.getIntent().getExtras();
		int errorCode = 0;
		if(bundle != null){
			errorCode = bundle.getInt("ERROR_CODE");
		}
		errorCode = 0 - errorCode;
		informationText = (TextView)findViewById(R.id.tv_content);
		informationText.setText(getString(R.string.warming_disconnect_msg)+"（"+getString(R.string.error_code)+":"+Integer.toString(errorCode)+")");
		btn_close = (Button)findViewById(R.id.closeBtn);
        OnClickListener theclick=new OnClickListener(){
            @Override
            public void onClick(View v){
                finish();
                //finishWithDialog("Canceled");
            }
        };
        btn_close.setOnClickListener(theclick);
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
	}
	
	private void releaseAll()
    {

    }
}
