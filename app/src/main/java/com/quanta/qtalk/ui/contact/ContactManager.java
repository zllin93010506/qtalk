package com.quanta.qtalk.ui.contact;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Hashtable;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class ContactManager 
{
 //   private static final String TAG             = "ContactManager";
    private static final String DEBUGTAG = "QTFa_ContactManager";
    public  static final String CUSTOM_PROTOCOL = "Quanta"; 
    public  static final int    PROTOCOL        =  android.provider.ContactsContract.CommonDataKinds.Im.PROTOCOL_CUSTOM;//-1
    public  static final int    TYPE            =  android.provider.ContactsContract.CommonDataKinds.Im.TYPE_OTHER;    //3
    private static Hashtable<String, String>    mQTNameAccountPair = new Hashtable<String, String>();
//    private static final boolean isDebug        = true;
    public static ArrayList<ContactQT> getAllContacts(Context context)
    {
        ArrayList<ContactQT> result = new ArrayList<ContactQT>();
        int count = loadAllContact(context,result);
        return result;
    }
    private static int loadAllContact(Context context, ArrayList<ContactQT> systemContactList)
    {
        Cursor data_cursor = context.getContentResolver()
                             .query(ContactsContract.Contacts.CONTENT_URI
                                  , null
                                  , null
                                  , null
                                  , null);
        int result =  0;
        if (data_cursor!=null && data_cursor.getCount()>0)
        {
            result =  data_cursor.getCount();
            while (data_cursor.moveToNext()) 
            {
                long contactId = Long.parseLong(data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Contacts._ID)));
                String display_name = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));
                ContactQT im = new ContactQT(contactId, "", loadContactPhoto(context.getContentResolver(), contactId),display_name);
                systemContactList.add(im);
            }
        }
        data_cursor.close();
        return result;
    }
    public static ArrayList<ContactQT> getQTContacts(Context context)
    {
        ArrayList<ContactQT> result = new ArrayList<ContactQT>();
        int count = loadContact(context,result);
        return result;
    }
    private static int loadContact(Context context, ArrayList<ContactQT> imList)
    {
        Cursor data_cursor = context.getContentResolver()
                             .query(ContactsContract.Data.CONTENT_URI
                                  , new String[] {ContactsContract.Data.CONTACT_ID,
                                               ContactsContract.Data.DISPLAY_NAME,
                                               ContactsContract.CommonDataKinds.Im.DATA,
                                               ContactsContract.CommonDataKinds.Im.PROTOCOL,
                                               ContactsContract.CommonDataKinds.Im.TYPE,
                                               ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL}
                                  , ContactsContract.CommonDataKinds.Im.PROTOCOL +" = ? AND "+
                                    ContactsContract.CommonDataKinds.Im.TYPE +" = ? AND "+
                                    ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL+" = ? AND "+
                                    ContactsContract.Data.MIMETYPE + " = ? "
                                  , new String[]{String.valueOf(PROTOCOL),
                                              String.valueOf(TYPE),
                                              CUSTOM_PROTOCOL,
                                              ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE}
                                  , null);
        int result =  0;
        if (data_cursor!=null && data_cursor.getCount()>0)
        {
            result =  data_cursor.getCount();
            while (data_cursor.moveToNext()) 
            {
                long contactId = Long.parseLong(data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Data.CONTACT_ID)));
                String display_name = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Data.DISPLAY_NAME));
                String im_data = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.CommonDataKinds.Im.DATA));
                ContactQT im = new ContactQT(contactId, im_data , loadContactPhoto(context.getContentResolver(), contactId),display_name);
                imList.add(im);
            }
        }
        if(data_cursor != null)
        {
            data_cursor.close();
            data_cursor = null;
        }
        resetQTNameAccountPair();
        return result;
    }
    
    public static long addContact(Context context, String setImName ,Bitmap setProfilePhoto,String setDisplayName)
    {
        return addContact(context,
                          new ContactQT(0, setImName , setProfilePhoto,setDisplayName));
    }
    
    public static long addContact(Context context, ContactQT contact)
    {
        long result = 0;
        if (contact.getDisplayName()!=null)
        {
            ContentValues values = new ContentValues();
            values.put(ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME, contact.getDisplayName());
            Uri rawContactUri = context.getContentResolver().insert(ContactsContract.RawContacts.CONTENT_URI, values);
            
            long rawContactId = ContentUris.parseId(rawContactUri); 
            values.clear();
            values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
            values.put(ContactsContract.Data.MIMETYPE,
                    StructuredName.CONTENT_ITEM_TYPE);
            values.put(StructuredName.DISPLAY_NAME, contact.getDisplayName());
            context.getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values); 
            Uri  imUri = Uri.withAppendedPath(rawContactUri,ContactsContract.Contacts.Data.CONTENT_DIRECTORY);
            values.clear();
            values.put(ContactsContract.CommonDataKinds.Im.TYPE, TYPE);
            values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE);
            if (contact.getQTAccount()!=null)
                values.put(ContactsContract.CommonDataKinds.Im.DATA, contact.getQTAccount());
            values.put(ContactsContract.CommonDataKinds.Im.PROTOCOL,PROTOCOL);
            values.put(ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL,CUSTOM_PROTOCOL);
            context.getContentResolver().insert(imUri, values);            
            result = ContentUris.parseId(rawContactUri);
            
            updateQTNameAccountPair(contact.getQTAccount(), contact.getDisplayName());
        }
        return result;
    }
    
    public static boolean updateContact(Context context, ContactQT contact)
    {
        boolean result = false;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>updateContact");
        // update contact by picking from existed contact.
        // check if this contact has viro account information
        Cursor data_cursor = context.getContentResolver()
                            .query(ContactsContract.Data.CONTENT_URI
                                 , new String[] {ContactsContract.Data.CONTACT_ID,
                                              ContactsContract.Data.DISPLAY_NAME,
                                              ContactsContract.CommonDataKinds.Im.DATA,
                                              ContactsContract.CommonDataKinds.Im.PROTOCOL,
                                              ContactsContract.CommonDataKinds.Im.TYPE,
                                              ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL}
                                 , ContactsContract.Data.CONTACT_ID +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.PROTOCOL +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.TYPE +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL+" = ? AND "+
                                   ContactsContract.Data.MIMETYPE + " = ? "
                                 , new String[]{String.valueOf(contact.getContactID()),
                                                String.valueOf(PROTOCOL),
                                                String.valueOf(TYPE),
                                                CUSTOM_PROTOCOL,
                                                ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE}
                                 , null);
        ContentValues values = new ContentValues();
        values.put(ContactsContract.Data.MIMETYPE,                        ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE);
        values.put(ContactsContract.CommonDataKinds.Im.PROTOCOL,          PROTOCOL);
        values.put(ContactsContract.CommonDataKinds.Im.DATA,              contact.getQTAccount());
        values.put(ContactsContract.CommonDataKinds.Im.TYPE,              TYPE);
        values.put(ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL,   CUSTOM_PROTOCOL);     
        values.put(ContactsContract.Data.IS_SUPER_PRIMARY, 1);
        if(data_cursor.moveToFirst()) // contact contains viro account --> update parameters
        {
            context.getContentResolver()
                   .update(ContactsContract.Data.CONTENT_URI
                          ,values
                          ,ContactsContract.Data.CONTACT_ID +" = ? AND "+
                           ContactsContract.CommonDataKinds.Im.PROTOCOL +" = ? AND "+
                           ContactsContract.CommonDataKinds.Im.TYPE +" = ? AND "+
                           ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL+" = ? AND "+
                           ContactsContract.Data.MIMETYPE + " = ? "
                         , new String[]{String.valueOf(contact.getContactID()),
                                        String.valueOf(PROTOCOL),
                                        String.valueOf(TYPE),
                                        CUSTOM_PROTOCOL,
                                        ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE});
            if(data_cursor != null)
            {
                data_cursor.close();
                data_cursor = null;
            }
            result = true;
        }else // contact has no viro account
        {
            // get raw_contact_id by querying contact list with contact_id
            Cursor phone_cursor = context.getContentResolver()
                                        .query(ContactsContract.Data.CONTENT_URI
                                                        ,new String[] {ContactsContract.Data.RAW_CONTACT_ID,ContactsContract.RawContacts.ACCOUNT_TYPE}
                                                        ,ContactsContract.Data.CONTACT_ID +" = ? "
                                                        ,new String[]{String.valueOf(contact.getContactID())}
                                                        ,null);
            if(phone_cursor.moveToFirst())
            {
                // insert new parameters by assigning RAW_CONTACT_ID
                long phone_raw_contact_id = Long.parseLong(phone_cursor.getString(phone_cursor.getColumnIndex(ContactsContract.Data.RAW_CONTACT_ID)));
                String account_type = phone_cursor.getString(phone_cursor.getColumnIndex(ContactsContract.RawContacts.ACCOUNT_TYPE));
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "account_type:"+account_type);
                result = "com.anddroid.contacts.sim".compareToIgnoreCase(account_type)==0?false:true;
                values.put(ContactsContract.Data.RAW_CONTACT_ID, phone_raw_contact_id);
                context.getContentResolver().insert(ContactsContract.Data.CONTENT_URI, values);
            }
            if(phone_cursor != null)
            {
                phone_cursor.close();
                phone_cursor = null;
            }
        }
        updateQTNameAccountPair(contact.getQTAccount(), contact.getDisplayName());
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==updateContact");
        return result;
    }
    public static Bitmap loadContactPhoto(ContentResolver photoCur, Long photoId )
    {
        Uri PhotoUri = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, photoId);
        try
        {
            InputStream input = ContactsContract.Contacts.openContactPhotoInputStream(photoCur, PhotoUri);
            if (input == null) {
                    return null;
            }
            else
            {
                Bitmap photo = BitmapFactory.decodeStream(input);
                try
                {
                    input.close();
                } catch (IOException e)
                {
                    Log.e(DEBUGTAG,"BitmapFactory.decodeStream - ",e);
                }
                return photo;//BitmapFactory.decodeStream(input);
            }
        }
        catch(Throwable err)
        {
            Log.e(DEBUGTAG,"loadContactPhoto - ",err);
            return null;
        }
    }
        
    public static ContactQT getContactByQtAccount(Context context , String qtAccount)
    {
        ContactQT result = null;
        Cursor data_cursor = context.getContentResolver()
                            .query(ContactsContract.Data.CONTENT_URI
                                 , new String[] {ContactsContract.Data.CONTACT_ID,
                                                 ContactsContract.Data.DISPLAY_NAME,
                                                 ContactsContract.CommonDataKinds.Im.DATA,
                                                 ContactsContract.CommonDataKinds.Im.PROTOCOL,
                                                 ContactsContract.CommonDataKinds.Im.TYPE,
                                                 ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL}
                                 , ContactsContract.CommonDataKinds.Im.DATA +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.PROTOCOL +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.TYPE +" = ? AND "+
                                   ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL+" = ? AND "+
                                   ContactsContract.Data.MIMETYPE + " = ? "
                                 , new String[]{qtAccount,
                                                String.valueOf(PROTOCOL),
                                                String.valueOf(TYPE),
                                                CUSTOM_PROTOCOL,
                                                ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE}
                                 , null);
        if (data_cursor!=null && data_cursor.getCount()>0)
        {
            if(data_cursor.moveToFirst())
            {
                long contactId = Long.parseLong(data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Data.CONTACT_ID)));
                String display_name = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Data.DISPLAY_NAME));
                String im_data = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.CommonDataKinds.Im.DATA));
                Bitmap bit_map = null;
                if(context.getContentResolver() != null)
                    bit_map = loadContactPhoto(context.getContentResolver(), contactId);
                if(display_name != null)
                    result = new ContactQT(contactId, im_data ,bit_map ,display_name);
                else
                    result = new ContactQT(contactId, im_data ,bit_map, "");
            }
        }
        if(data_cursor != null)
        {
            data_cursor.close();
            data_cursor = null;
        }
        return result;
    }
    
    public static void resetQTNameAccountPair()
    {
        mQTNameAccountPair.clear();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"resetQTNameAccountPair");
    }
    
    private static void updateQTNameAccountPair(String qtName, String qtAccount)
    {
        mQTNameAccountPair.remove(qtAccount);
        mQTNameAccountPair.put(qtAccount,qtName);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"updateQTNameAccountPair");
    }
    
    private static void setQTNameAccountPair(String qtAccount, String qtName)
    {
        mQTNameAccountPair.put(qtAccount, qtName);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setQTNameAccountPair");
    }
    
    private static final String getQTNameAccountPair(String qtAccount)
    {
        String result= null;
        if(mQTNameAccountPair != null)
            result = mQTNameAccountPair.get(qtAccount);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getQTNameAccountPair:"+result);
        return result;
    }
    public static String getNameByQtAccount(Context context , String qtAccount)
    {
        String result = null;
        Cursor data_cursor = null;
        if(getQTNameAccountPair(qtAccount) != null)
            result = getQTNameAccountPair(qtAccount);
        else
        {        
            data_cursor = context.getContentResolver()
                                .query(ContactsContract.Data.CONTENT_URI
                                     , new String[] {ContactsContract.Data.CONTACT_ID,
                                                     ContactsContract.Data.DISPLAY_NAME,
                                                     ContactsContract.CommonDataKinds.Im.DATA,
                                                     ContactsContract.CommonDataKinds.Im.PROTOCOL,
                                                     ContactsContract.CommonDataKinds.Im.TYPE,
                                                     ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL}
                                     , ContactsContract.CommonDataKinds.Im.DATA +" = ? AND "+
                                       ContactsContract.CommonDataKinds.Im.PROTOCOL +" = ? AND "+
                                       ContactsContract.CommonDataKinds.Im.TYPE +" = ? AND "+
                                       ContactsContract.CommonDataKinds.Im.CUSTOM_PROTOCOL+" = ? AND "+
                                       ContactsContract.Data.MIMETYPE + " = ? "
                                     , new String[]{qtAccount,
                                                    String.valueOf(PROTOCOL),
                                                    String.valueOf(TYPE),
                                                    CUSTOM_PROTOCOL,
                                                    ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE}
                                     , null);
            if (data_cursor!=null && data_cursor.getCount()>0)
            {
                if(data_cursor.moveToFirst())
                {
                    String display_name = data_cursor.getString(data_cursor.getColumnIndex(ContactsContract.Data.DISPLAY_NAME));
                    if(display_name != null)
                    {
                        result = display_name;
                        setQTNameAccountPair(qtAccount,display_name);
                    }
                    else
                        result = null;
                }
            }
        }
        if(data_cursor != null)
        {
            data_cursor.close();
            data_cursor = null;
        }
        return result;
    }
}