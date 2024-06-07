package com.quanta.qtalk.ui.history;

import android.provider.CallLog;


public class CallHistoryInfo
{
    public static final int        CALL_TYPE_MISSED   = CallLog.Calls.MISSED_TYPE;
    public static final int        CALL_TYPE_OUTGOING = CallLog.Calls.OUTGOING_TYPE;
    public static final int        CALL_TYPE_INCOMING = CallLog.Calls.INCOMING_TYPE;
    public static final String     CALL_MISSED = "missed";
    public static final String     CALL_OUTGOING = "=>";
    public static final String     CALL_INCOMING = "<=";
    public static final String     CALL_NUMBER_KEY = "NUMBER";
    
        String  mImData = null;
        int     mCallType = -1;
        String  mDate = null;
        String  mDuring = null;
        String  mName = null;
        String  mAlias = "";
        String  mRole = null;
        String  mUid  = null;
        String  mTitle = null;
        int     mMedia = -1;
        int     mKeyId = -1;
        int 	mIncomingNum = 0;
        
        public CallHistoryInfo(String imData, String name, String alias, String title, String role, String uid, int callType , String date ,String during, int isVideo, int keyId, int IncomingNum)
        {
            mImData      = new String(imData);
            mCallType    = new Integer(callType);
            mDate        = new String(date);
            mDuring      = new String(during);
            mName        = new String(name);
            mAlias       = new String((alias==null)?"":alias);
            mRole        = new String(role);
            mUid         = new String(uid);
            mTitle       = new String(title);
            mMedia       = new Integer(isVideo);
            mKeyId       = new Integer(keyId);
            mIncomingNum = new Integer(IncomingNum);
        }
        public void setQTAccount(String imData)
        {
            mImData = new String(imData);
        }
        public String getQTAccount()
        {
            return mImData;
        }
        public int getCallType()
        {
            return mCallType;
        }
        
        public String getDate()
        {
            return mDate;
        }
        
        public String getDuring()
        {
        	return mDuring;
        }
        public String getName() { return mName; }
        public String getAlias()
    {
        return mAlias;
    }
        public String getRole()
        {
        	return mRole;
        }
        public String getUid()
        {
        	return mUid;
        }
        public String getTitle()
        {
        	return mTitle;
        }
        public int getMedia()
        {
            return mMedia;
        }
        
        public int getKeyId()
        {
            return mKeyId;
        }
        
        public int getIncomingNum()
        {
            return mIncomingNum;
        }
}
