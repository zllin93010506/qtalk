package com.quanta.qtalk.ui;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import com.quanta.qtalk.Flag;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.R;
import com.quanta.qtalk.ui.AbstractUiListActivity;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.ViroActivityManager;
import com.quanta.qtalk.ui.notification.NotificationInfo;
import com.quanta.qtalk.ui.notification.NotificationParser;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import com.quanta.qtalk.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;

public class NotificationActivity extends AbstractUiListActivity{
	
	private static final String DEBUGTAG = "NotificationActivity";
	private Handler mHandler = new Handler();
	private Context mContext;
	ArrayList<NotificationInfo> temp_list;
	NotificationListAdapter mNotificationListAdapter;
	NotificationView nv = null;
	Button Return = null;
	private final String notification_url = Hack.getStoragePath()+"notification.xml";
	
    final Runnable NotificationUpdateCheck = new Runnable() {
        public void run() {
        		if (Flag.Notification.getState())
        		{
        			updateNotification();
        			Flag.Notification.setState(false);
        		}
        		mHandler.postDelayed(NotificationUpdateCheck, 10000);
        	}
        };
	
	@Override
	public void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		setContentView(com.quanta.qtalk.R.layout.layout_notification);
		File file = new File(notification_url);
		if(file.exists())
		{
			try {
				temp_list = NotificationParser.parseProvisionNotification(notification_url);
			} catch (ParserConfigurationException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			} catch (SAXException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			}
		}
		OnClickListener theclick=new OnClickListener(){
	            @Override
	            public void onClick(View v) {
	                synchronized(NotificationActivity.this)
	                {
						if(v.getId()==R.id.return_btn) 
						{
							finish();
						}
	                }
	            }//End of onclick
	        };
	        Return = (Button) findViewById(R.id.return_btn);
	        Return.setOnClickListener(theclick);
	}
	
	@Override
    public void onDestroy()
    {
        super.onDestroy();
        releaseAll();
    }
	
	private class NotificationListAdapter extends BaseAdapter 
    {
        public NotificationListAdapter(Context context) 
        {
            mContext = context;
        }
        public int getCount() {
            return temp_list.size();
//                return mNames.length;
        }
        public Object getItem(int position) {
            return position;
        }
        public long getItemId(int position) {
            return position;
        }
        public View getView(int position, View convertView, ViewGroup parent) {
        	NotificationView nv = null;
            //Log.d(DEBUGTAG,"convertView"+convertView +", position:"+position);
            nv = new NotificationView(mContext,
            		temp_list.get(position).getString());
            return nv;
        }
     }//END of class NotificationListAdapter
    
    private class NotificationView extends LinearLayout 
    {
           public NotificationView(final Context context,final String message) 
           {
               super(context);
               //this.setOrientation(HORIZONTAL);
               
           }//end of NotificationView constructor 
    }//END of NotificationView
    
    @Override
    protected void onListItemClick(ListView l, final View v, int position, final long id)
    {
    	
    }//END of onListItemClick
    
    @Override
	public void onResume()
	{
		super.onResume();
		if(temp_list != null)
        {
            NotificationListAdapter mNotificationListAdapter = new NotificationListAdapter(this);
            setListAdapter(mNotificationListAdapter);
        }
		ViroActivityManager.setActivity(this);
	}
    
    private void releaseAll()
	{
		mNotificationListAdapter = null;
		nv = null;
		temp_list = null;
		mContext = null;
		mHandler.removeCallbacks(NotificationUpdateCheck);
	}
    
    public void updateNotification()
    {	
    		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"updateNotification");
			setContentView(com.quanta.qtalk.R.layout.layout_notification);
    }

}
