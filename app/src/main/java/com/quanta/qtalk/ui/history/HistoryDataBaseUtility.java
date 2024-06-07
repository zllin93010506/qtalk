package com.quanta.qtalk.ui.history;

import java.util.ArrayList;
import java.util.List;

import com.quanta.qtalk.ui.HistoryActivity;
import com.quanta.qtalk.ui.ProvisionSetupActivity;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Handler;
import android.os.Message;
import com.quanta.qtalk.util.Log;

public class HistoryDataBaseUtility
{
//    private static final String TAG         = "LogDataBaseUtility";
    private static final String DEBUGTAG = "QTFa_HistoryDataBaseUtility";
//    private static final boolean ENABLE_DEBUG = false;
    public static final String KEY_ID       = "_id";
    public static final String KEY_NUM      = "num";
    public static final String KEY_NAME     = "name";
    public static final String KEY_ALIAS     = "alias";
    public static final String KEY_TITLE    = "title";
    public static final String KEY_ROLE     = "role";
    public static final String KEY_UID      = "uid";
    public static final String KEY_TYPE     = "type";
    public static final String KEY_DATE     = "date";
    public static final String KEY_DUR      = "during";
    public static final String KEY_MEDIA    = "media";
    public static int size = 1; 

    private static final String DB_NAME     = "CallHistory.db";
    private static final String DB_TABLE    = "table_CallHistory";
    private static final int    DB_ROW_NUMBER   = 60;
    private static final int DB_VERSION     = 2;
    private static Context mContext = null;
    private static final String DB_CREATE = "CREATE TABLE " + DB_TABLE + " ("
            + KEY_ID + " INTEGER PRIMARY KEY," + KEY_NUM + " TEXT,"
            + KEY_NAME + " TEXT,"+ KEY_ALIAS + " TEXT," + KEY_TITLE + " TEXT,"+ KEY_ROLE + " TEXT," + KEY_UID + " TEXT,"
            + KEY_TYPE + " INTEGER," + KEY_DATE + " TEXT," + KEY_DUR + " TEXT,"+ KEY_MEDIA + " INTEGER)";
    private SQLiteDatabase mSQLiteDatabase = null;
    private DatabaseHelper mDatabaseHelper = null;
    private static ICallHistory mCallBack = null;
    private static List<CallHistoryInfo> mCallLogInfo = new ArrayList<CallHistoryInfo>();
    private boolean isOpen = false;
    private static Handler mHandler = null;

    public interface ICallHistory
    {
        public void updateCallHistory();
    }
    private static class DatabaseHelper extends SQLiteOpenHelper
    {
        DatabaseHelper(Context context)
        {
            super(context, DB_NAME, null, DB_VERSION);
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "DatabaseHelper");
        }

        @Override
        public void onCreate(SQLiteDatabase db)
        {
        	//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onCreate:"+db.getPath());
            db.execSQL(DB_CREATE);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)
        {
        	//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onUpgrade oldVersion="+oldVersion +" newVersion="+newVersion);
            if (oldVersion < 2) {
                try {
                    db.execSQL("ALTER TABLE "+DB_TABLE+" ADD COLUMN "+KEY_ALIAS+" TEXT default ''");
                } catch (SQLiteException ex) {
                    Log.w(DEBUGTAG, "Altering " + DB_TABLE + ": " + ex.getMessage());
                }
            }
            //db.execSQL("DROP TABLE IF EXISTS notes");
            //onCreate(db);
        }
        
//        @Override
//        public void onTerminate(SQLiteDatabase db)
//        {
//            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onUpgrade");
//            db.execSQL("DROP TABLE IF EXISTS notes");
//            onCreate(db);
//        }
    }

    public HistoryDataBaseUtility(Context context)
    {
    	//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "new LogDataBaseUtility");
        mContext = context;
        if(mCallLogInfo.size()==0)
        	updateCallLogInfo();
    }
    
    private void updateCallLogInfo()
    {
    	//Log.d(DEBUGTAG,"==>updateCallLogInfo");
    	Cursor data_cursor =  null;
        if (mDatabaseHelper==null)
        {
            mDatabaseHelper = new DatabaseHelper(mContext);
        }
    	if (mSQLiteDatabase==null)
        {
            mSQLiteDatabase = mDatabaseHelper.getWritableDatabase();
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "open - mSQLiteDatabase:"+mSQLiteDatabase);
        }
    	if(mSQLiteDatabase != null)
            data_cursor = mSQLiteDatabase.query(DB_TABLE,
                                                new String[] { KEY_ID,
                                                               KEY_NUM,
                                                               KEY_NAME,
                                                               KEY_ALIAS,
                                                               KEY_TITLE,
                                                               KEY_ROLE,
                                                               KEY_UID,
                                                               KEY_TYPE,
                                                               KEY_DATE,
                                                               KEY_DUR,
                                                               KEY_MEDIA},
                                                null, null, null, null, null, null);
        if (data_cursor!=null && data_cursor.getCount()>0)
        {
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"++++++++++++++++++++++++++++++++");
            while (data_cursor.moveToNext()) 
            {
            	int call_type = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_TYPE)));
        		String caller_number = data_cursor.getString(data_cursor.getColumnIndex(KEY_NUM));
        		String caller_name = data_cursor.getString(data_cursor.getColumnIndex(KEY_NAME));
                String caller_alias = data_cursor.getString(data_cursor.getColumnIndex(KEY_ALIAS));
        		String caller_title = data_cursor.getString(data_cursor.getColumnIndex(KEY_TITLE));
        		String caller_role = data_cursor.getString(data_cursor.getColumnIndex(KEY_ROLE));
        		String caller_uid = data_cursor.getString(data_cursor.getColumnIndex(KEY_UID));
        		String call_date = data_cursor.getString(data_cursor.getColumnIndex(KEY_DATE));
        		String call_during = data_cursor.getString(data_cursor.getColumnIndex(KEY_DUR));
        		int call_media = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_MEDIA)));
        		int key_id = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_ID)));
        		//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"caller_number:"+caller_number+", caller_title:"+caller_title+", call_type:"+call_type +", call_date:"+call_date+" ,key_id:"+key_id);
        		//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"++++++++++++++++++++++++++++++++");
        		CallHistoryInfo info = new CallHistoryInfo(caller_number,  caller_name, caller_alias, caller_title, caller_role, caller_uid, call_type, call_date, call_during, call_media,key_id,0);
        		mCallLogInfo.add(0,info);
            		
            	//}
            }
        }
        Log.d(DEBUGTAG,"<==updateCallLogInfo mCallLogInfo.size():"+mCallLogInfo.size());
    }
    
    public DatabaseHelper open(boolean iswritable) throws SQLException
    {
        if (mDatabaseHelper==null)
        {
            mDatabaseHelper = new DatabaseHelper(mContext);
        }
        if (mSQLiteDatabase==null)
        {
            
            if (iswritable)
                mSQLiteDatabase = mDatabaseHelper.getWritableDatabase();
            else
                mSQLiteDatabase = mDatabaseHelper.getReadableDatabase();
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "open - mSQLiteDatabase:"+mSQLiteDatabase);
        }
        isOpen = true;
        return mDatabaseHelper;
    }
    public void close()
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "close - mSQLiteDatabase:"+mSQLiteDatabase);
        if (mSQLiteDatabase!=null && mSQLiteDatabase.isOpen())
        {
            mSQLiteDatabase.close();
        }
        if (mDatabaseHelper!=null && isOpen)
        {
            mDatabaseHelper.close();
        }
        mSQLiteDatabase = null;
        mDatabaseHelper = null;
        mCallBack = null;
        mContext = null;
        isOpen = false;
    }

    public long addData(final String number, final String name, final String alias , final String title, final String role, final String uid, final int type, final String date, final String duringTime, final boolean isVideo)
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>addData - mSQLiteDatabase:"+mSQLiteDatabase +", number:"+number +", name:"+name +", alias:"+alias +", type:"+type +", date:"+date +", isVideo:" +isVideo);
        ContentValues initialValues = new ContentValues();
        initialValues.put(KEY_NUM, number);
        initialValues.put(KEY_NAME, name);
        initialValues.put(KEY_ALIAS, alias);
        initialValues.put(KEY_TITLE, title);
        initialValues.put(KEY_ROLE, role);
        initialValues.put(KEY_UID, uid);
        initialValues.put(KEY_TYPE, type);
        initialValues.put(KEY_DATE, date);
        initialValues.put(KEY_DUR, duringTime);
        initialValues.put(KEY_MEDIA, isVideo?1:0);
        long key_id = mSQLiteDatabase.insert(DB_TABLE, KEY_ID, initialValues);
        mCallLogInfo.add(0, new CallHistoryInfo(number, name, alias, title, role, uid, type, date, duringTime, isVideo?1:0,(int)key_id,0));
        Log.d(DEBUGTAG,"mCallLogInfo.size():"+mCallLogInfo.size());
        if(mCallLogInfo.size() > DB_ROW_NUMBER)
        {
            //delete the last one
            int select_id = -1;
            CallHistoryInfo info = mCallLogInfo.get(mCallLogInfo.size()-1);
            if(info != null)
                select_id = info.mKeyId;
            mCallLogInfo.remove(info);//(mCallLogInfo.size()-1);
            if(select_id >= 0)
                deleteData(select_id);
        }
        if(mCallBack != null && mHandler!=null){
        	Message msg = new Message(); 
        	msg.what = HistoryActivity.UPDATE_HISTORY;
            mHandler.sendMessage(msg);
        }
        return key_id;
    }
    public static void setCallBack(ICallHistory cb, Handler handler)
    {
        mCallBack = cb;
        mHandler = handler;
    }
    
    public List<CallHistoryInfo> getAllData()
    {
        ArrayList<CallHistoryInfo> result = new ArrayList<CallHistoryInfo>();
        int count = 0;
        int data_limit = 20; //call list limit number
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "getAllData - mSQLiteDatabase:"+mSQLiteDatabase);
        Cursor data_cursor =  null;
        if(mSQLiteDatabase != null)
            data_cursor = mSQLiteDatabase.query(DB_TABLE, 
                                                new String[] { KEY_ID, 
                                                               KEY_NUM,
                                                               KEY_NAME,
                                                               KEY_ALIAS,
                                                               KEY_TITLE,
                                                               KEY_ROLE,
                                                               KEY_UID,
                                                               KEY_TYPE, 
                                                               KEY_DATE,
                                                               KEY_DUR,
                                                               KEY_MEDIA}, 
                                                null, null, null, null, null, null);
        if (data_cursor!=null && data_cursor.getCount()>0)
        {
            count =  data_cursor.getCount();
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"-----------------------------");
            String caller_number_old = "0";
            int call_type_old = 10;
            int IncomingNum = 1;
            while (data_cursor.moveToNext() && count > 0) 
            {
            	int call_type = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_TYPE)));
            	//if (call_type == 3)
            	//{
            		String caller_number = data_cursor.getString(data_cursor.getColumnIndex(KEY_NUM));
            		String caller_name = data_cursor.getString(data_cursor.getColumnIndex(KEY_NAME));
                    String caller_alias = data_cursor.getString(data_cursor.getColumnIndex(KEY_ALIAS));
            		String caller_title = data_cursor.getString(data_cursor.getColumnIndex(KEY_TITLE));
            		String caller_role = data_cursor.getString(data_cursor.getColumnIndex(KEY_ROLE));
            		String caller_uid = data_cursor.getString(data_cursor.getColumnIndex(KEY_UID));
            		String call_date = data_cursor.getString(data_cursor.getColumnIndex(KEY_DATE));
            		String call_during = data_cursor.getString(data_cursor.getColumnIndex(KEY_DUR));
            		int call_media = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_MEDIA)));
            		int key_id = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_ID)));
            		//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"caller_number:"+caller_number+", caller_title:"+caller_title+", call_type:"+call_type +", call_date:"+call_date+" ,key_id:"+key_id);
            		//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"-----------------------------");
            		if (caller_number.contentEquals(caller_number_old) && call_type == 3 && call_type_old == 3)
            		{
                    	result.remove(0);
                    	IncomingNum = IncomingNum + 1;
                    	CallHistoryInfo info = new CallHistoryInfo(caller_number, caller_name, caller_alias, caller_title, caller_role, caller_uid, call_type, call_date, call_during, call_media,key_id,IncomingNum);
                		result.add(0,info);
                		//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"caller_number is "+caller_number+"   caller_number_old is "+caller_number_old+"   IncomingNum is "+IncomingNum);
            		}
            		else
            		{
            			CallHistoryInfo info = new CallHistoryInfo(caller_number,  caller_name, caller_alias, caller_title, caller_role, caller_uid, call_type, call_date, call_during, call_media,key_id,0);
                		result.add(0,info);
                		IncomingNum = 1;
            		}
            		caller_number_old = caller_number;
            		call_type_old = call_type;
            	//}
            }
//            if(mCallLogInfo.size() == 0)
//            {
//                mCallLogInfo.addAll(result);
//            }
        }
        size = result.size();
        if (size > data_limit) //call list only show 20 records
        {
        	for (int i=size-1; i > data_limit-1; i-- )
        	{
        		result.remove(i);
        	}
        }
        if(data_cursor != null)
            data_cursor.close();
        return result;
    }

//    public boolean updateData(long rowId, int num, String name)
//    {
//        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "updateData");
//        ContentValues args = new ContentValues();
//        args.put(KEY_NUM, num);
//        args.put(KEY_NAME, name);
//
//        return mSQLiteDatabase.update(DB_TABLE, args, KEY_ID + "=" + rowId,
//                null) > 0;
//    }
    
//  another manner to delete element in db
//  public boolean deleteData(long keyId)
//  {
//      Log.d(DEBUGTAG, "deleteData:"+keyId);
//      return mSQLiteDatabase.delete(DB_TABLE, KEY_ID + "=" + keyId, null) > 0;
//  }
    public void deactivate(){
    	Log.d(DEBUGTAG,"deactivate");
    	mCallLogInfo.clear();
    	if(mDatabaseHelper==null){
    		if(mContext==null){
    			Log.d(DEBUGTAG,"mContext null");
    		}
    		mDatabaseHelper = new DatabaseHelper(mContext);
    	}
    	SQLiteDatabase delete_db = mDatabaseHelper.getWritableDatabase();
		delete_db.delete(DB_TABLE, null, null);
		delete_db.close();
    }
    public void deleteData(int keyId)
    {
        //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"deleteData:"+keyId);
        mSQLiteDatabase.execSQL("DELETE FROM " + DB_TABLE + " WHERE " + KEY_ID +"=" + keyId);
    }
    
    public void deleteData()
    {
	    Cursor data_cursor = mSQLiteDatabase.query(DB_TABLE, 
	                                            new String[] { KEY_ID, 
	                                                           KEY_NUM,
	                                                           KEY_TYPE, 
	                                                           KEY_DATE,
	                                                           KEY_DUR,
	                                                           KEY_MEDIA}, 
	                                            null, null, null, null, null, null);
	    data_cursor.moveToFirst();
        int cscnt = 0;
        for( cscnt = 0; cscnt < data_cursor.getCount(); cscnt++)
        {
    		int key_id = Integer.valueOf(data_cursor.getString(data_cursor.getColumnIndex(KEY_ID)));
        	data_cursor.moveToNext();
        	mSQLiteDatabase.execSQL("DELETE FROM " + DB_TABLE + " WHERE " + KEY_ID +"=" + key_id);
        }
        data_cursor.close();
    }
    
    public int deleteData(long rowId) throws IndexOutOfBoundsException
    {
        //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"deleteData - 2:"+rowId + ",mCallLogInfo.size:"+mCallLogInfo.size());
        CallHistoryInfo info = null;
        int key_id = -1;
        
        if(mCallLogInfo.size() > 0)
        {
            info = mCallLogInfo.get((int)rowId);
            mCallLogInfo.remove(mCallLogInfo.get((int) rowId));
            if (info != null && info.mKeyId >= 0)
                key_id = info.mKeyId;
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"info:"+info+", key_id:"+key_id);
            if(key_id >= 0)
                mSQLiteDatabase.delete(DB_TABLE, KEY_ID + "=" + key_id, null);
        }
            return mCallLogInfo.size();
    }
    
  /*  public void deactivation()
    {
    	SQLiteDatabase delete_db = mDatabaseHelper.getWritableDatabase();
		delete_db.delete(DB_TABLE, null, null);
		delete_db.close();
    }*/
    
    public static int getCallHistorySize()
    {
        int result = -1;
        if(mCallLogInfo.size() > 0)
            return mCallLogInfo.size();
        return result;
    }
    public void clearData()
    {
        if (mSQLiteDatabase!=null)
        {
            while(mCallLogInfo.size() > 0)
            {
                deleteData();
                mCallLogInfo.remove(0);
            }
            mCallLogInfo.clear();
            close();
//            mSQLiteDatabase.close();
        }
//        if(mContext != null)
//            mContext.deleteDatabase(DB_NAME);
    }
}
