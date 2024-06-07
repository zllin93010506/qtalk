package com.quanta.qtalk.ui.contact;

import java.io.InputStream;

import android.content.ContentUris;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
public class ContactQT
{
    private String  mImData = null;
    private Bitmap  mBitmap = null;
    private long    mPhotoID = 0;
    private String  mDisplayName = null;
    private long    mID = 0; //unique contact ID
    int mLocation = 0; // 0: local, 1:global
    int isBlocked = 0;
    int PX2Key;
    
    public ContactQT(long contactID, String imData, Bitmap bitmap , String displayName)
    {
        mID          = contactID;
        if(imData != null)
            mImData      = new String(imData);
        else
            mImData      = new String("");
        if(displayName != null)
            mDisplayName = new String(displayName);
        else
            mDisplayName = new String("");
        mBitmap      = bitmap;
    }
    ContactQT(long contactID, String imData, Bitmap bitmap , String displayName, int location)
    {
        mID          = contactID;
        if(imData != null)
            mImData      = new String(imData);
        else
            mImData      = new String("");
        if(displayName != null)
            mDisplayName = new String(displayName);
        else
            mDisplayName = new String("");
        
        mLocation = location; 
        mBitmap      = bitmap;
    }
//    ContactQT(long contactID, String imData, long photoID , String displayName)
//    {
//        mID          = contactID;
//        mImData      = new String(imData);
//        mDisplayName = new String(displayName);
//        mPhotoID     = photoID;
//        mBitmap      = null;
//    }
    
    public void setLocation(int location)
    {
    	this.mLocation = location;
    }
    public int getLocation()
    {
    	return this.mLocation;
    }
    
    public long getContactID()
    {
        return mID;
    }
    
    public void setQTAccount(String imData)
    {
        mImData = new String(imData);
    }
    public String getQTAccount()
    {
        return mImData;
    }
    
    public Bitmap getBitmap()
    {
        return mBitmap;
    }
    public Bitmap getBitmap(Context context)
    {
        if (mBitmap==null && mPhotoID!=0)
        {
            Uri photo_uri = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, mPhotoID);
            InputStream input = ContactsContract.Contacts.openContactPhotoInputStream(context.getContentResolver(), photo_uri);
            if (input != null) 
            {
                mBitmap = BitmapFactory.decodeStream(input);
            }
        }
        return mBitmap;
    }

    public void setDisplayName(String displayName) {
        mDisplayName = new String(displayName);
    }
    public String getDisplayName() {
        return mDisplayName;
    }
}