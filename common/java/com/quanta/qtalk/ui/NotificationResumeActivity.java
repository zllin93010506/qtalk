package com.quanta.qtalk.ui;

import com.quanta.qtalk.VersionDefinition;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public class NotificationResumeActivity extends Activity
{

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
    }
    @Override
    public void onStart()
    {
        super.onStart();
        Activity last_activity = ViroActivityManager.getLastActivity();
        if(!QTReceiver.engine(this,false).isLogon())
        {
            if(VersionDefinition.CURRENT == VersionDefinition.SOFT_PHONE)
            {
                //Intent start_intent = new Intent();
                //start_intent.setClassName(this, SIPLoginActivity.class.getName());
            	Intent start_intent =last_activity.getIntent();
                start_intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                startActivity(start_intent);
                finish();
                return;
            }
            else if(VersionDefinition.CURRENT == VersionDefinition.PROVISION)
            {
                //Intent start_intent = new Intent();
                //start_intent.setClassName(this, ProvisionLoginActivity.class.getName());
            	if (last_activity == null)
            	{
            		Intent start_intent = new Intent();
                    start_intent.setClassName(this, LoginRetryActivity.class.getName());
                    start_intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                    startActivity(start_intent);
                    finish();
            	}
            	else
            	{
	                Intent start_intent =last_activity.getIntent();
	                start_intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
	                startActivity(start_intent);
	                finish();
            	}
                return;
            }
        }
        if(last_activity != null)
        {
            Intent start_intent = last_activity.getIntent();
            start_intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(start_intent);
            finish();
        }else
        {
            Intent start_intent = new Intent();
            start_intent.setClassName(this, ContactsActivity.class.getName());
            start_intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(start_intent);
            finish();
        }
    }
}
