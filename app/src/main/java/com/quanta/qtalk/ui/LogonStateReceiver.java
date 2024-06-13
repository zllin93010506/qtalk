package com.quanta.qtalk.ui;

import com.quanta.qtalk.R;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import com.quanta.qtalk.util.Log;

public class LogonStateReceiver  extends BroadcastReceiver 
{
    @Override
    public void onReceive(final Context context, Intent intent) 
    {
        final String action = intent.getAction();
        if (action!=null && action.equals(QTReceiver.ACTION_LOGON_STATE))
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
                                        title+" 線上",                       //title
                                        logon_id,                              //content
                                        new_intent);                           //intent
                    }else
                    {
                        setNotification(context,
                        				R.drawable.icn_disconnected,
                                    //    R.drawable.ic_login_error,
                                        logon_id,
                                        title+" 離線",
                                        logon_msg,
                                        new_intent);
                    }
                }
            }
        }
    }
    @SuppressWarnings("deprecation")
	public static void setNotification(Context context,final int id_icon,final String tickText,final String title,final String content,Intent intent)
    {
        NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(QTReceiver.NOTIFICATION_ID_LOGON);
        
        Notification.Builder builder = new Notification.Builder(context);
        builder.setWhen(System.currentTimeMillis());
        builder.setSmallIcon(id_icon);
        builder.setTicker(tickText);

        Notification notification = builder.build();
        notification.flags |= Notification.FLAG_SHOW_LIGHTS;
        notification.flags |= Notification.FLAG_NO_CLEAR; // keep notification on unless logout
     
        notificationManager.notify(QTReceiver.NOTIFICATION_ID_LOGON, notification);
    }
}
