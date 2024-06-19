package com.quanta.qtalk.ui;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;

public class VideoEndPopWindow extends Activity
{

	Handler mHandler=new Handler();
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(com.quanta.qtalk.R.layout.layout_videoend_pop);

        final Runnable runnableThread = new Runnable() {
            public void run() {
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
