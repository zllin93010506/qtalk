package com.quanta.qtalk;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.contact.QTContact2;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import com.quanta.qtalk.util.Log;

public class QtalkDB extends SQLiteOpenHelper {
	private static final String DATABASE_NAME = "QTALKDB";
	private static final String PHONEBOOK_TABLE_NAME = "QTALKDB_PHONEBOOK";
	private static final String AUTHENTICATION_TABLE = "QTALKDB_AUTHENTICATION";
	private static final String PROVISION_TABLE = "QTALKDB_PROVISION";
	private static final int DATABASE_VERSION = 3;
	//public static final String TAG_NAME = "Name";
	//public static final String TAG_PHONENUMBER = "PhoneNumber";
	//public static final String TAG_LOCATION = "Location";
	private static final String DEBUGTAG = "QTFa_QtalkDB";
	private static final String TAG_BUDDYLIST = "phonebook";
	private static final String TAG_BUDDY = "person";
	
	public static final String TAG_NAME = "name";
	public static final String TAG_ALIAS = "alias";
	public static final String TAG_ROLE = "role";
	public static final String TAG_PHONENUMBER = "number";
	public static final String TAG_UID = "uid";
	public static final String TAG_ONLINE = "online"; 
	public static final String TAG_RINGTONE = "Ringtone";
	public static final String TAG_TITLE = "title";
//	private static final String TAG_BLOCK = "Block";
	private static final String TAG_IMAGE = "image";
//	private static final String TAG_LOCATION = "Location";
	public final String TAG_SESSION = "Session";

	public final String TAG_SERVER = "Server";
	public final String TAG_VERSION = "Version";
	public final String TAG_CONFIRM = "Confirm";
	public final String TAG_AUTHENTICATION = "Authentication";
	public QtalkDB() {
		super(QtalkApplication.getAppContext(), DATABASE_NAME, null, DATABASE_VERSION);
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		Log.d(DEBUGTAG,"onCreate");
		String CREATE_PB_TABLE = "create table if not exists "
			+ PHONEBOOK_TABLE_NAME +" (id integer primary key autoincrement, "
			//+ "role text not null, name text not null, number text not null, image text not null);";
			+ "role text not null, name text not null, alias text not null, number text not null,  uid text not null, online text not null, title text not null, image text not null);";

		String CREATE_AT_TABLE = "create table if not exists "
				+ AUTHENTICATION_TABLE +" (id integer primary key autoincrement, "
				+ " Session text not null, Key text not null, Server text not null, Uid text not null, Type text not null,  Version text not null);";
		
		String CREATE_PT_TABLE = "create table if not exists "
				+ PROVISION_TABLE +" (id integer primary key autoincrement, "
				+ "lastPhonebookFileTime text not null, lastProvisionFileTime text not null);";
		
		if(ProvisionSetupActivity.debugMode) Log.d("EventsData", "onCreate: " + CREATE_PB_TABLE+" & "+CREATE_AT_TABLE+" & "+CREATE_PT_TABLE);
		db.execSQL(CREATE_PB_TABLE);
		db.execSQL(CREATE_AT_TABLE);
		db.execSQL(CREATE_PT_TABLE);
	}
	


	@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		Log.d(DEBUGTAG,"onUpgrade");
		if (oldVersion < 2) {
			final String DROP_TABLE = "DROP TABLE IF EXISTS " + AUTHENTICATION_TABLE;

			db.execSQL(DROP_TABLE);
			onCreate(db);
		}
		if (oldVersion < 3) {
			db.execSQL("ALTER TABLE " + PHONEBOOK_TABLE_NAME + " ADD COLUMN " + TAG_ALIAS + " TEXT default ''");
		}
	}
	
	int insertPBEntry(ContentValues values)
	{
		Cursor rdb = this.getReadableDatabase().query(PHONEBOOK_TABLE_NAME, 
				null, TAG_PHONENUMBER +" LIKE ?", new String[] { "%"+values.getAsString(this.TAG_PHONENUMBER)+"%"}, null, null, null);
		
		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount()"+rdb.getCount());
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			rdb.close();
			return -1;
		}
		
		SQLiteDatabase db = this.getWritableDatabase();
	    db.insertOrThrow(QtalkDB.PHONEBOOK_TABLE_NAME, null, values);
	    rdb.close();
		return 0;
	}
	
	
	
	
	int insertATEntry(ContentValues values)
	{
		Cursor rdb = this.getReadableDatabase().query(AUTHENTICATION_TABLE, 
				null, TAG_SESSION +" LIKE ?", new String[] { "%"+values.getAsString(this.TAG_SESSION)+"%"}, null, null, null);
		
		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount()"+rdb.getCount());
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			rdb.close();
			return -1;
		}
		
		SQLiteDatabase db = this.getWritableDatabase();
	    db.insertOrThrow(QtalkDB.AUTHENTICATION_TABLE, null, values);
	    rdb.close();
		return 0;
	}
	
	
	
	public int insertPTEntry(String lastPhonebookFileTime, String lastProvisionFileTime)  /* Create activation information storage */
	{
		SQLiteDatabase db = this.getWritableDatabase();
		Cursor cs1 = returnAllPTEntry();
		cs1.moveToFirst();
      	int cscnt = 0;
      	if (cs1.getCount() > 1)
      	{
      		for( cscnt = 0; cscnt < cs1.getCount(); cscnt++)
      		{
	      	//	int key_id = Integer.valueOf(cs1.getString(cs1.getColumnIndex(lastPhonebookFileTime)));
	      		cs1.moveToNext();
	      		db.execSQL("DELETE FROM " + PROVISION_TABLE + " WHERE " + "lastPhonebookFileTime=" + lastPhonebookFileTime);
      		}
      	}
      	cs1.close();
      	
		Cursor rdb = this.getReadableDatabase().query(PROVISION_TABLE, null, 
				"lastPhonebookFileTime" +" LIKE ?", new String[] { "%"+lastPhonebookFileTime+"%"}, null, null, null);
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			rdb.close();
			return -1;
		}
//		SQLiteDatabase db = this.getWritableDatabase();
	    ContentValues values = new ContentValues();
	    values.put("lastPhonebookFileTime", lastPhonebookFileTime);
	    values.put("lastProvisionFileTime", lastProvisionFileTime);
	    db.insertOrThrow(PROVISION_TABLE, null, values);
	    rdb.close();
		return 0;
	}
	
	
	
	
	
	
	public int insertATEntry(String Session, String Key, String Server, String Uid, String Type, String Version)  /* Create activation information storage */
	{
		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Session:"+Session+",   Key:"+Key+",   Server:"+Server+",   Uid:"+Uid+",   Version:"+Version);
		Cursor rdb = this.getReadableDatabase().query(AUTHENTICATION_TABLE, null, 
				TAG_SESSION +" LIKE ?", new String[] { "%"+Session+"%"}, null, null, null);
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			rdb.close();
			return -1;
		}
		SQLiteDatabase db = this.getWritableDatabase();
	    ContentValues values = new ContentValues();
	    values.put(TAG_SESSION, Session);
	    values.put("Key", Key);
	    //values.put(TAG_SERVER, "https://"+Server);
	    values.put(TAG_SERVER, Server);
	    values.put("Uid", Uid);
	    values.put("Type", Type);
	    values.put("Version", Version);
	    db.insertOrThrow(AUTHENTICATION_TABLE, null, values);
	    rdb.close();
		return 0;
	}
	
	
	int insertPBEntry(String Name, String PhoneNumber, String Location )
	{
		Cursor rdb = this.getReadableDatabase().query(PHONEBOOK_TABLE_NAME, null, 
				TAG_PHONENUMBER +" LIKE ?", new String[] { "%"+PhoneNumber+"%"}, null, null, null);
		
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			return -1;
		}
			
//		SQLiteDatabase db = this.getWritableDatabase();
		
	    ContentValues values = new ContentValues();
	    values.put(TAG_NAME, Name);
	    values.put(TAG_PHONENUMBER, PhoneNumber);
	    values.put("Ringtone", "");
	    values.put("Block", "");
	    values.put("Image", "");
	    values.put("Location", Location);
	    values.put("PX2PBKey", "");
		return insertPBEntry(values);
	}

	


	public int insertPBEntry(ArrayList<QTContact2> contact_list)  
	{
		Cursor rdb = this.getReadableDatabase().query(PHONEBOOK_TABLE_NAME, null, 
				TAG_PHONENUMBER +" LIKE ?", new String[] { "%"+TAG_PHONENUMBER+"%"}, null, null, null);
		if( rdb.getCount() >= 1)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
			rdb.close();
			return -1;
		}
		SQLiteDatabase db = this.getWritableDatabase();
	    ContentValues values = new ContentValues();
	    values.put(TAG_PHONENUMBER, contact_list.get(0).getPhoneNumber());
	    values.put(TAG_NAME, contact_list.get(0).getName());
	    String alias = contact_list.get(0).getAlias();
		values.put(TAG_ALIAS, (alias==null)?"":alias);
	    values.put(TAG_ROLE, contact_list.get(0).getRole());
	    values.put(TAG_IMAGE, contact_list.get(0).getImage());
	    //values.put("uid", contact_list.get(0).getUid());
	    //values.put("online", contact_list.get(0).getUid());
	    db.insertOrThrow(PHONEBOOK_TABLE_NAME, null, values);
	    rdb.close();
		return 0;
	}
	
	
	
	
	
	
	public void importProvisioningPhonebook(ArrayList<QTContact2> contact_list)
	{
		SQLiteDatabase write_db = this.getWritableDatabase();
		write_db.beginTransaction();
		
		if (returnAllPBEntry().getCount() > 0)
		{
			
			String CREATE_PB_TABLE = "create table if not exists "
					+ PHONEBOOK_TABLE_NAME +" (id integer primary key autoincrement, "
					+ "role text not null, name text not null, alias text not null, number text not null,uid text not null, online text not null, title text not null, image text not null);";
			String sql = "drop table " + PHONEBOOK_TABLE_NAME;
			try {
				write_db.execSQL(sql);
			} catch (SQLException e) {
				Log.e(DEBUGTAG,"importProvisioningPhonebook",e);
			}
			write_db.execSQL(CREATE_PB_TABLE);
		}

		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"size:"+contact_list.size());
		for( int cnt = 0; cnt < contact_list.size(); cnt++)
		{
			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Role:"+contact_list.get(cnt).getRole()+" Name:"+contact_list.get(cnt).getName()+" PhoneNumber:"+contact_list.get(cnt).getPhoneNumber()+" Image:"+contact_list.get(cnt).getImage());
			Cursor rdb = this.getReadableDatabase().query(PHONEBOOK_TABLE_NAME, null, 
					TAG_PHONENUMBER +" LIKE ?", new String[] { "%"+contact_list.get(cnt).getPhoneNumber()+"%"}, null, null, null);
			if( rdb.getCount() >= 1)
			{
				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"rdb.getCount() >= 1");
				//rdb.close();
				continue;
			}

			rdb.close();
			ContentValues values = new ContentValues();
		    values.put(TAG_ROLE, contact_list.get(cnt).getRole());
		    values.put(TAG_NAME, contact_list.get(cnt).getName());
		    String alias = contact_list.get(cnt).getAlias();
			values.put(TAG_ALIAS, (alias==null)?"":alias);
		    values.put(TAG_PHONENUMBER, contact_list.get(cnt).getPhoneNumber());
		    values.put(TAG_IMAGE, contact_list.get(cnt).getImage());
		    values.put(TAG_UID, contact_list.get(cnt).getUid());
		    values.put(TAG_ONLINE, contact_list.get(cnt).getOnline());
		    values.put(TAG_TITLE, contact_list.get(cnt).getTitle());
		    write_db.insertOrThrow(PHONEBOOK_TABLE_NAME, null, values);
		   
		}
		write_db.setTransactionSuccessful();
		write_db.endTransaction();
	}
	
	
	
	public void deactiveation()
	{
		Log.d(DEBUGTAG,"deactiveation");
		
		SQLiteDatabase delete_db = this.getWritableDatabase();
		
		delete_db.delete(AUTHENTICATION_TABLE, null, null);
		delete_db.delete(PHONEBOOK_TABLE_NAME, null, null);
		delete_db.delete(PROVISION_TABLE, null, null);
		DeleteATEntry(AUTHENTICATION_TABLE, TAG_SESSION);
		Log.d(DEBUGTAG,"deactiveation "+delete_db.getAttachedDbs().getClass());
		delete_db.close();
	}
	

	
	public Cursor queryPBEntry(String PhoneNumber)
	{
		Cursor cs = this.getReadableDatabase().query(PHONEBOOK_TABLE_NAME, null, TAG_PHONENUMBER +" LIKE ?", new String[] { "%"+PhoneNumber+"%"}, null, null, PhoneNumber);
		return cs;
	}
	
	public Cursor queryATEntry(String Session)
	{
		Cursor cs = this.getReadableDatabase().query(AUTHENTICATION_TABLE, null, TAG_SESSION +" LIKE ?", new String[] { "%"+Session+"%"}, null, null, Session);
		return cs;
	}
	
	public Cursor queryATEntry(String table, String tag) /* Create function of query for each table */
	{
		Cursor cs = this.getReadableDatabase().query(table, null, tag +" LIKE ?", new String[] { "%"+tag+"%"}, null, null, tag);
		return cs;
	}
	
	public Cursor queryPTEntry(String lastPhonebookFileTime)
	{
		Cursor cs = this.getReadableDatabase().query(PROVISION_TABLE, null, "lastPhonebookFileTime" +" LIKE ?", new String[] { "%"+lastPhonebookFileTime+"%"}, null, null, lastPhonebookFileTime);
		return cs;
	}
	
	public Cursor returnAllPBEntry()
	{
		return queryPBEntry("");
	}
	
	public Cursor returnAllATEntry()
	{
		return queryATEntry("");
	}
	
	public Cursor returnAllPTEntry()
	{
		return queryPTEntry("");
	}
	
	int DeletePBEntry(String PhoneNumber)
	{
		return this.getWritableDatabase().delete(PHONEBOOK_TABLE_NAME, TAG_PHONENUMBER+" LIKE ?", new String[] { "%"+PhoneNumber+"%"});
	}
	
	public int DeleteATEntry(String table, String tag)
	{
		return this.getWritableDatabase().delete(table, tag+" LIKE ?", new String[] { "%"+tag+"%"});
	}
	
	
	public static ArrayList<QTContact2> parseProvisionPhonebook(String provisionConfigPath) throws ParserConfigurationException, SAXException, IOException
	//public static ContentValues parseProvisionPhonebook(String provisionConfigPath) throws ParserConfigurationException, SAXException, IOException
    {
		//TODO 1. initialize an ArrayList here and insert every PB entry into it
		//     2. insert this ArrayList into Database
		ArrayList<QTContact2> temp_list = new ArrayList<QTContact2>();
//		ContentValues cv = new ContentValues();
        DocumentBuilder documentBuilder  = null;
        Document document        = null;
        NodeList elements        = null;
        int iElementLength= 0;
        documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        if(documentBuilder != null)
        {
            document = documentBuilder.parse(new File(provisionConfigPath));
        }
        if(document != null)
        {
        	document.getDocumentElement().normalize();
            elements  = document.getElementsByTagName(TAG_BUDDYLIST);
        }
        if(elements != null)
        {
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
                return null;
            
//            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"iElementLength:"+iElementLength);
            for (int i = 0; i < iElementLength ; i++) 
            {
            	
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
//                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"properties.getLength():"+properties.getLength());
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    NodeList property_list = properties_node.getChildNodes();
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name != null)
                    {
                        if(properties_node_name.equalsIgnoreCase(TAG_BUDDY))
                        {   	
                        	QTContact2 temp = new QTContact2();
                            //parsing ftp server
                        	//if(ProvisionSetupActivity.debugMode)	Log.d(DEBUGTAG,"properties_length:"+ property_list.getLength());
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                            	
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_ROLE))
                                {
                                	temp.setRole(checkNodeValue(property_list_node.getFirstChild()));
                                	//cv.put(TAG_BLOCK, checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_NAME))
                                {
                                	temp.setName(checkNodeValue(property_list_node.getFirstChild()));
                                	//cv.put(TAG_NAME, checkNodeValue(property_list_node.getFirstChild()));
                                }
								else if(name.equalsIgnoreCase(TAG_ALIAS))
								{
									temp.setAlias(checkNodeValue(property_list_node.getFirstChild()));
									//cv.put(TAG_NAME, checkNodeValue(property_list_node.getFirstChild()));
								}
                                else if(name.equalsIgnoreCase(TAG_PHONENUMBER))
                                {
                                	temp.setPhoneNumber(checkNodeValue(property_list_node.getFirstChild()));
                                	//cv.put(TAG_PHONENUMBER, checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_IMAGE))
                                {
                                    temp.setImage(checkNodeValue(property_list_node.getFirstChild()));
                                    //cv.put(TAG_IMAGE, checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_UID))
                                {
                                	temp.setUid(checkNodeValue(property_list_node.getFirstChild()));
                                    //cv.put(TAG_IMAGE, checkNodeValue(property_list_node.getFirstChild()));
                                } 
                                else if(name.equalsIgnoreCase(TAG_ONLINE))
                                {
                                    temp.setOnline(checkNodeValue(property_list_node.getFirstChild()));
                                    //cv.put(TAG_IMAGE, checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_TITLE))
                                {
                                    temp.setTitle(checkNodeValue(property_list_node.getFirstChild()));
                                    //cv.put(TAG_IMAGE, checkNodeValue(property_list_node.getFirstChild()));
                                } 
                              
                            }
                            temp_list.add(temp); 
                        }
                    }
                    
                }
                
            }
            
        }
        /*Log.d(TAG,"temp_list_size:"+ temp_list.size());
        for (int i=0; i< temp_list.size(); i++)
        {
        	Log.d(TAG,"temp_list"+ temp_list.get(i).getName());
        }   */
        return temp_list;
    }
    private static String checkNodeValue(Node firstChild)
    {
        String result = "";
        try
        {
            if(firstChild != null)
            {
                String temp_result = firstChild.getNodeValue();
                if(temp_result != null && temp_result.trim().length() > 0)
                    result = temp_result;
            }
            //if(ProvisionSetupActivity.debugMode)	Log.d(DEBUGTAG,"checkNodeValue:"+result);
        }
        catch(Exception e)
        {
            Log.e(DEBUGTAG,"checkNodeValue Error",e);
        }
        return result;
    }

	public static String getString(Cursor cs, String column) {
		int index = cs.getColumnIndex(column);
		if(index<0)
			return "";
		return cs.getString(index);
	}
}
