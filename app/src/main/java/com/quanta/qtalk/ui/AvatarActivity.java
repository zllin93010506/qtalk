package com.quanta.qtalk.ui;

import com.quanta.qtalk.Flag;

import android.os.Bundle;
import android.os.Handler;
import com.quanta.qtalk.util.Log;


public class AvatarActivity extends AbstractVolumeActivity
{
	ContactsActivity contactsActivity = new ContactsActivity();
//	private int avatarRunnable = 0;
//	private static final int AVATAR_SUCCESS = 7;
//	private static final int AVATAR_FAIL = 8;
//	private String TAG="AvatarActivity";
    private static final String DEBUGTAG = "QTFa_AvatarActivity";
	Handler mHandler=new Handler();
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(com.quanta.qtalk.R.layout.layout_avatar_popupwindows);

        final Runnable runnableThread = new Runnable() {
            public void run() {
            	Log.d(DEBUGTAG,"==>runnableThread");
            	Flag.AvatarSuccess.setState(false);
                finish();
            }
        };
        mHandler.postDelayed(runnableThread, 1000);
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
		super.onDestroy();
	}
}
