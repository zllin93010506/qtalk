package com.quanta.qtalk.ui.history;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

import android.R;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.ParseException;
import android.os.Build;
import android.os.Bundle;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.contact.ContactManager;
import com.quanta.qtalk.ui.contact.ContactQT;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public class HistoryManager
{
 //   private static final String TAG = "LogManager";
    private static final String DEBUGTAG = "QTFa_HistoryManager";
    private static HistoryDataBaseUtility mLogDataBaseUtility = null;
    //// Lawrence modifiied
    @SuppressLint("SimpleDateFormat")
 	private static String getDateCurrentTimeZone(long timestamp) {
    	Calendar c = Calendar.getInstance();
        c.setTimeInMillis(timestamp);
        Date d = c.getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("MM/dd HH:mm");
        return sdf.format(d);
     }
    private static String getDateCurrentTimeZone2(long timestamp){
    	long alertTimeLong = timestamp;
    	Date dt = new Date(alertTimeLong);
    	SimpleDateFormat sdf = new SimpleDateFormat("MM/dd HH:mm",Locale.getDefault());
    	String alertTimeString = sdf.format(dt);
    	return alertTimeString;
    }
    public static void addCallLog(Context context,final ContactQT contact, final String phoneNumber, final String Name, final String Alias, final String Title, final String Role, final String Uid, final int callType,final long startTime,final long nowTime,final String duringTime, final boolean isVideoCall)
    {	
        if (mLogDataBaseUtility==null)
        {
            mLogDataBaseUtility = new HistoryDataBaseUtility(context);
            mLogDataBaseUtility.open(true);
        }
        if(Hack.isStation()&&(Build.VERSION_CODES.O<=Build.VERSION.SDK_INT)){
            Bundle calllogBundle = new Bundle();
            calllogBundle.putString("phoneNumber", phoneNumber);
            calllogBundle.putString("Name", Name);
            calllogBundle.putString("Alias", Alias);
            calllogBundle.putString("Title", Title);
            calllogBundle.putString("Role", Role);
            calllogBundle.putString("Uid", Uid);
            calllogBundle.putInt("callType", callType);
            calllogBundle.putLong("startTime", startTime);
            calllogBundle.putLong("nowTime", nowTime);
            calllogBundle.putString("duringTime", duringTime);
            calllogBundle.putBoolean("isVideoCall", isVideoCall);
            Intent intent = new Intent("android.intent.action.vc.call_log.update");
            intent.putExtras(calllogBundle);
            context.sendBroadcast(intent);
        }
        if (mLogDataBaseUtility!=null)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"addCallLog:"+phoneNumber+" "+Name+" "+Alias+" "+Title+" "+Role+" "+Uid+" "+callType+" "+startTime);
        
            Date date = new Date(); 
            //DateFormat fullFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            //DateFormat fullFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm");
            DateFormat fullFormat = new SimpleDateFormat("MM/dd HH:mm");
            //DateFormat fullFormat = new SimpleDateFormat("MM-dd HH:mm");
            String startDate1 = getDateCurrentTimeZone(startTime);
            String startDate2 = getDateCurrentTimeZone2(startTime);
            if(startTime == 0){
            	startDate1 = fullFormat.format(date);
            }
            Log.d(DEBUGTAG,"starttime : "+ startTime +" startDate1 : "+startDate1+"startDate2 : "+startDate2+" now : "+fullFormat.format(date));
            
            ContactQT contact_qt = ContactManager.getContactByQtAccount(context, phoneNumber);
            String name = null;
            if(contact_qt != null)
                name = ContactManager.getContactByQtAccount(context, phoneNumber).getDisplayName();
            
            long result = mLogDataBaseUtility.addData(phoneNumber, Name, Alias, Title, Role, Uid, callType, startDate1, duringTime ,isVideoCall);
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "addCallLog - result:"+result);
        }
    }

 
    
    ////
//    public static void addCallLog(Context context,final ContactQT contact,final String phoneNumber,final int callType,final long startTime)
//    {
//        Log.d(DEBUGTAG,"addCallLog:"+contact+" "+phoneNumber+" "+callType+" "+startTime);
//        ContentValues values = new ContentValues();
//        if (contact!=null)
//        {
//            values.put(CallLog.Calls.CACHED_NAME, contact.getDisplayName());
//            values.put(CallLog.Calls.CACHED_NUMBER_TYPE, ContactManager.TYPE);
//            values.put(CallLog.Calls.CACHED_NUMBER_LABEL, ContactManager.CUSTOM_PROTOCOL);
//            
//            //values.put(CallLog.Calls., contact.getContactID());
//            People.markAsContacted(context.getContentResolver(), contact.getContactID());
//        }
//        values.put(CallLog.Calls.NUMBER, phoneNumber+"@"+ContactManager.CUSTOM_PROTOCOL);
//
//        values.put(CallLog.Calls.NEW, (startTime > 0)?1:0);
//        //values.put(CallLog.Calls.CACHED_NUMBER_LABEL, phoneNumber); 
//        values.put(CallLog.Calls.TYPE , callType); //(incoming, outgoing or missed) CallLog.Calls.MISSED_TYPE
//        if (startTime==0)
//        {
//            values.put(CallLog.Calls.DURATION,0);
//            values.put(CallLog.Calls.DATE,System.currentTimeMillis() );
//        }else
//        {
//            values.put(CallLog.Calls.DURATION,(System.currentTimeMillis()-startTime)/1000);
//            values.put(CallLog.Calls.DATE,startTime);
//        }
//        Uri rawContactUri = context.getContentResolver().insert(CallLog.Calls.CONTENT_URI, values);
//    }
//    public static Uri addCall(CallerInfo ci, Context context, String number,
//            boolean isPrivateNumber, int callType, long start, int duration) {
//        final ContentResolver resolver = context.getContentResolver();
//
//        if (TextUtils.isEmpty(number)) {
//            if (isPrivateNumber) {
//                number = CallerInfo.PRIVATE_NUMBER;
//            } else {
//                number = CallerInfo.UNKNOWN_NUMBER;
//            }
//        }
//
//        ContentValues values = new ContentValues(5);
//
//        if (number.contains("&"))
//                number = number.substring(0,number.indexOf("&"));
//        values.put(Calls.NUMBER, number);
//        values.put(Calls.TYPE, Integer.valueOf(callType));
//        values.put(Calls.DATE, Long.valueOf(start));
//        values.put(Calls.DURATION, Long.valueOf(duration));
//        values.put(Calls.NEW, Integer.valueOf(1));
//        if (ci != null) {
//            values.put(Calls.CACHED_NAME, ci.name);
//            values.put(Calls.CACHED_NUMBER_TYPE, ci.numberType);
//            values.put(Calls.CACHED_NUMBER_LABEL, ci.numberLabel);
//        }
//
//        if ((ci != null) && (ci.person_id > 0)) {
//            People.markAsContacted(resolver, ci.person_id);
//        }
//
//        Uri result = resolver.insert(Calls.CONTENT_URI, values);
//
//        if (result != null) { // send info about call to call meter
//                final Intent intent = new Intent(ACTION_CM_SIP);
//                intent.putExtra(EXTRA_SIP_URI, result.toString());
//                // TODO: add provider
//                // intent.putExtra(EXTRA_SIP_PROVIDER, null);
//                context.sendBroadcast(intent);
//        }
//
//        return result;
//    }
}
