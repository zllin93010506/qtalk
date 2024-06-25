package com.quanta.qtalk.ui;

import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.database.Cursor;
import android.widget.Toast;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.ui.contact.ContactManager;
import com.quanta.qtalk.ui.contact.ContactQT;
import com.quanta.qtalk.util.Log;

public abstract class AbstractContactsActivity extends AbstractUiListActivity
{
//    private static final String TAG = "AbstractContactsActivity";
    private static final String DEBUGTAG = "QTFa_AbstractContactsActivity";
    protected ArrayList<ContactQT> mContactQTList = null;
    protected void onStart()
    {
    	super.onStart();
    	try
        {
            mContactQTList = ContactManager.getQTContacts(this);
            QtalkDB qtalkdb = QtalkDB.getInstance(this);
            Cursor cs = qtalkdb.returnAllPBEntry();
            cs.moveToFirst();
            for(int cscnt = 0; cscnt < cs.getCount(); cscnt++)
            {
                String name = QtalkDB.getString(cs, "name");
                if(name.trim().equals("")) {
                    cs.moveToNext();
                    continue;
                }
                String phone_number = QtalkDB.getString(cs, "number");
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"TestName:'"+ name +"' PhoneNumber:"+phone_number);
            	ContactQT contact = new ContactQT( 0, phone_number, null, name);
            	contact.setLocation(1);
            	mContactQTList.add(contact);
            	cs.moveToNext();
            }
            cs.close();
            qtalkdb.close();
            
        }catch(Throwable e)
        {
            Log.e(DEBUGTAG,"getQTContacts",e);
        }
    }
    protected void onStop()
    {
    	super.onStop();
    	
    }
    
    @Override
    protected void onResume()
    {
        Log.d(DEBUGTAG,"onResume");
        super.onResume();
        
    }
    @Override
    protected void onPause()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onPause");
        super.onPause();
    }
    @SuppressLint("ShowToast")
	protected boolean makeCall(String remoteID,boolean enableVideo)
    {
        boolean result = false;
        //Use call engine to Make call
        try
        {
            QTReceiver.engine(this,false).makeCall(null,remoteID, enableVideo);
        } catch (FailedOperateException e) 
        {
            Log.e(DEBUGTAG,"makeCall",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
        return result;
    }
    @SuppressLint("ShowToast")
	protected boolean makeSeqCall(String remoteID,boolean enableVideo,String DisplayName){
        boolean result = false;
        //Use call engine to Make call
        try
        {
            QTReceiver.engine(this,false).makeSeqCall(null, remoteID, enableVideo, DisplayName);
        } catch (FailedOperateException e) 
        {
            Log.e(DEBUGTAG,"makeCall",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
        return result;
    }
    protected void exitQT()
    {
        QTReceiver.finishQT(this);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"playSoundLogout");
        RingingUtil.playSoundLogout(this);
    }
}
