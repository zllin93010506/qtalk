package com.quanta.qtalk.ui;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.ui.contact.ContactManager;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import com.quanta.qtalk.util.Log;

public class OutgoingCallReceiver extends BroadcastReceiver 
{
//    private final static String TAG = "OutgoingCallReceiver";
    private static final String DEBUGTAG = "QTFa_OutgoingCallReceiver";
    private static final Handler mHandler     = new Handler();

    public static final String EXTRA_ALREADY_CALLED = "android.phone.extra.ALREADY_CALLED";
    public static final String EXTRA_ORIGINAL_URI = "android.phone.extra.ORIGINAL_URI";
    @Override
    public void onReceive(final Context context, Intent intent) {
        final String action = intent.getAction();
        final String number = getResultData();
        final String full_number = intent.getStringExtra(EXTRA_ORIGINAL_URI);
        final int    code = this.getResultCode();
        final boolean enableVideo = true;
        if (action!=null && action.equals(Intent.ACTION_NEW_OUTGOING_CALL) && !intent.getBooleanExtra("android.phone.extra.ALREADY_CALLED",false) &&
            number!=null && full_number!=null) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "act=" + action + " num=" + number + " fnum=" + full_number +" code="+code );
            QtalkEngine engine = QTReceiver.engine(context,false);
            if (engine!=null && engine.isLogon())
            {
                final int tel_index = full_number.indexOf("tel:");
                final int at_index = full_number.indexOf("%40");//Index of '@'
                if (at_index>0 && full_number.endsWith(ContactManager.CUSTOM_PROTOCOL))
                {
                    setResultData(null);
                    mHandler.post(new Runnable()
                    {
                        public void run()
                        {
                            String remoteID = full_number.substring(tel_index+4,at_index);
//                            Intent new_intent = new Intent();
//                            new_intent.setAction(QTReceiver.ACTION_DIALER);
//                            new_intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//                            Bundle bundle = new Bundle();
//                            Log.d(DEBUGTAG, "remoteID=" + remoteID);
//                            bundle.putString(QTReceiver.KEY_REMOTE_ID , remoteID);
//                            new_intent.putExtras(bundle);
//                            context.startActivity(new_intent);
                            try {
                                QTReceiver.engine(context,false).makeCall(null,remoteID, enableVideo);
                            } catch (FailedOperateException e) {
                                Log.e(DEBUGTAG,"makeCall",e);
                            }
                        }
                    });
                }else if (engine.replaceSystemOutgoingCall())
                {
                    setResultData(null);
                    mHandler.post(new Runnable()
                    {
                        public void run()
                        {
//                            Intent new_intent = new Intent();
//                            new_intent.setAction(QTReceiver.ACTION_DIALER);
//                            new_intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//                            Bundle bundle = new Bundle();
//                            bundle.putString(QTReceiver.KEY_REMOTE_ID , number);
//                            bundle.putInt(QTReceiver.KEY_CALL_TYPE , code);
//                            new_intent.putExtras(bundle);
//                            context.startActivity(new_intent);
                            try {
                                QTReceiver.engine(context,false).makeCall(null,number, enableVideo);
                            } catch (FailedOperateException e) {
                                Log.e(DEBUGTAG,"makeCall",e);
                            }
                        }
                    });
                }
            }
        }
    }
}
