package com.quanta.qtalk.ui;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import com.quanta.qtalk.Flag;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public class LogonStateReceiver  extends BroadcastReceiver 
{
    private static final String DEBUGTAG = "QSE_LogonStateReceiver";
    public static final String ACTION_LOGON_STATE       = "com.quanta.qtalk.ui.logon.state";
    @Override
    public void onReceive(final Context context, Intent intent) 
    {
        final String action = intent.getAction();
        if (action!=null && action.equals(ACTION_LOGON_STATE))
        {
            final Bundle bundle = intent.getExtras();
            if (bundle!=null)
            {
                final String logon_id = bundle.getString(QTReceiver.KEY_LOGON_ID);
                final String logon_msg = bundle.getString(QTReceiver.KEY_LOGON_MSG);
                final String app_name = context.getString(R.string.app_name);
                final String title = app_name;
                Intent new_intent = new Intent();
                new_intent.setAction(QTReceiver.ACTION_RESUME);
                if (QTReceiver.isExited())
                {
                    final NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
                    notificationManager.cancel(QTReceiver.NOTIFICATION_ID_LOGON);
                    notificationManager.cancel(QTReceiver.NOTIFICATION_ID_CALLLOG);
                }else
                {
                    if (bundle.getBoolean(QTReceiver.KEY_LOGON_STATE,false))
                    {
                        setNotification(context,
                        				R.drawable.icn_connected,
                                //        R.drawable.ic_login_ok,                //Icon
                                        logon_id,                              //notification msg
                                        title+" 線上");                           //intent
                    }else
                    {
                        setNotification(context,
                        				R.drawable.icn_disconnected,
                                    //    R.drawable.ic_login_error,
                                        logon_id,
                                        title+" 離線");
                    }
                }
            }
        }
    }

    private static final String CHANNEL_ID = "PhoneApp";
    private static final NotificationChannel mChannel= new NotificationChannel(CHANNEL_ID,
            CHANNEL_ID, NotificationManager.IMPORTANCE_DEFAULT);;
    public static Notification buildNotification(Context context,final int id_icon,final String tickText,final String title)
    {
        int icon = id_icon;

        String _title = title;
        if(icon!=R.drawable.icn_connected && icon!=R.drawable.icn_disconnected) {
            _title = context.getString(R.string.app_name);
            if(Flag.SIPRunnable.getState()) {
                icon = R.drawable.icn_connected;
                _title= _title+" 線上";
            } else {
                icon = R.drawable.icn_disconnected;
                _title= _title+" 線上";
            }
        }

        ((NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE)).createNotificationChannel(mChannel);
        mChannel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
        mChannel.setSound(null, null);
        Notification notification = new Notification.Builder(context, CHANNEL_ID)
                .setWhen(System.currentTimeMillis())
                .setSmallIcon(icon)
                .setPriority(Notification.PRIORITY_LOW)
                .setTicker(tickText)
                .setContentText("").build();
        notification.flags |= Notification.FLAG_SHOW_LIGHTS;
        notification.flags |= Notification.FLAG_NO_CLEAR;

        return notification;
    }

	public static void setNotification(Context context,final int id_icon,final String tickText,final String title)
    {
        NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(QTReceiver.NOTIFICATION_ID_LOGON);

        Notification notification = buildNotification(context, id_icon, tickText, title);

        notificationManager.notify(QTReceiver.NOTIFICATION_ID_LOGON, notification);
    }

    public static String logon_msg = "";
    public static boolean logon_state = false;

    private static boolean isNative = true;

    public static void notifyLogonState(final Context context,final boolean state,final String loginID,final String msg, boolean isNative)
    {
        Log.d(DEBUGTAG, "notifyLogonState state:"+state+", loginID:"+loginID+", msg:"+msg + ", isNative:"+isNative  + ", context==null:"+(context==null));
        String login_id =  loginID;
        if(login_id==null)
            login_id = "QSPT-VC";
        logon_state = state;
        if(state)
            logon_msg = login_id;
        else
            logon_msg = msg;

        if(context!=null) {
            Intent intent = new Intent();
            if(!Hack.isPhone()) {
                intent.setAction(LogonStateReceiver.ACTION_LOGON_STATE);//TODO: Resume Applicaiton
                // create bundle & Assign Bundle to Intent
                Bundle bundle = new Bundle();
                bundle.putBoolean(QTReceiver.KEY_LOGON_STATE , state);
                bundle.putString(QTReceiver.KEY_LOGON_ID , login_id);
                bundle.putString(QTReceiver.KEY_LOGON_MSG , msg);
                intent.putExtras(bundle);
                // start Activity Class
                context.sendBroadcast(intent);
            }
            else {
                intent.setAction(LauncherReceiver.ACTION_VC_LOGON_STATE);
                context.sendBroadcast(intent);
                Log.d(DEBUGTAG, "send " + LauncherReceiver.ACTION_VC_LOGON_STATE);
            }
        }
        LogonStateReceiver.isNative = isNative;
    }

    public static void notifyLogonState(final Context context,final boolean state,final String loginID,final String msg)
    {
        notifyLogonState(context, state, loginID, msg, true);
    }

    public static boolean isNotifyLogonStateNative()
    {
        return isNative;
    }
}