package com.quanta.qtalk.ui;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
//import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
//import android.database.Cursor;
import android.database.SQLException;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils.TruncateAt;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.LinearLayout.LayoutParams;

import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkMain;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.DrawableTransfer;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.ui.history.CallHistoryInfo;
import com.quanta.qtalk.ui.history.HistoryDataBaseUtility;
import com.quanta.qtalk.ui.history.HistoryDataBaseUtility.ICallHistory;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

public class HistoryActivity extends AbstractContactsActivity implements ICallHistory
{
 //   private static final String     TAG = "HistoryActivity";
    private static final String DEBUGTAG = "QTFa_HistoryActivity";
    HistoryDataBaseUtility          mDataBaseAdapter;
//    public Cursor                   mCursor;
    ContactListAdapter              mContactListAdapter;    
    CallLogView                     cv = null;
    public static final int 		UPDATE_HISTORY = 0;
    public static final int         CONFIGURE_MENU_ITEM = Menu.FIRST;
    public static final int         ADD_LOG_MENU_ITEM = CONFIGURE_MENU_ITEM+1;
    public static final int         CLEAR_DB_MENU_ITEM = CONFIGURE_MENU_ITEM + 2;
    public static final int         EXIT_MENU_ITEM = CONFIGURE_MENU_ITEM + 3;
    public static int         photoSize=180;
    private static int        picSize=100;
    protected List<CallHistoryInfo>     mCallLogList = null;
    private static final String TAG_FAMILY = "family";
    private static final String TAG_NURSE = "nurse";
    private static final String TAG_PATIENT = "patient";
    private static final String TAG_BROADCAST = "mcu";
    private static final String TAG_UNKNOWN = "unknown";
    private String missImage = Hack.getStoragePath()+"QOCAMyimage.jpg";
	private String missName = "Unknown User";
	private String missRole= TAG_FAMILY;
	private String missRole_UI = "Unknown";
	private int CallSize;
    Drawable Temp1 = null;
    private ImageView mContactPhoto;
    private ImageView mCallTypePic;
    ImageButton Careplan = null, Canlendar = null, Profile = null;
	Button Contacts = null;
	Button Contact,Keypad,Contact_info;
	private int screenwidth = 200;
	private int screenheight = 100;
	private int screenSize = 0;
	
	public static int dpi = 0, ScreenSize = 0;
	
	private AlertDialog tts_readmore_popout = null;
	Button close_btn =null;
	private boolean newNotification = false;
	
    QtalkEngine   engine;
    QtalkSettings qtalksetting ;
	
//    QuickAction mQuickAction;
    /**
     * Remember our context so we can use it when constructing views.
     */
    private Context mContext;
    
    private BroadcastReceiver receiver = new BroadcastReceiver(){

		@SuppressLint("NewApi")
		@Override
		public void onReceive(Context context, Intent intent) {
			if(intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)){
				//finish();
				//overridePendingTransition(0, 0);
				Log.d(DEBUGTAG,"reveive ACTION_CLOSE_SYSTEM_DIALOGS");
			}else if(intent.getAction().equals(LauncherReceiver.PHONE_NEW_NOTIFICATION)){
				Bundle bundle = intent.getExtras();
				int count = bundle.getInt("NEW_ITEM");
				if(count >0){
					//Notification.setBackground(getResources().getDrawable(R.drawable.btn_notification_new_call));
					Log.d(DEBUGTAG,"there is new notifycation");
				}else{
					Log.d(DEBUGTAG,"there is no new notifycation");
				}
				mHandler.removeCallbacks(CheckNotificationCB);
			}
		}
    };
    
	private Handler mHandler = new Handler();
	private Handler mHisDBHandler = new Handler()
	{
	        public void handleMessage(Message msg)
	        {
	          switch(msg.what)
	          {
	          	case UPDATE_HISTORY:
	          		updateCallHistory();
	          		break;
	          }
	        }
	};
		final Runnable CheckNotificationCB = new Runnable() {
			@Override
			public void run() {
				if(!newNotification){
					Intent intent = new Intent();
			        intent.setAction(LauncherReceiver.PHONE_QUERY_NOTIFICATION);
			        sendBroadcast(intent);
			        mHandler.postDelayed(CheckNotificationCB, 5000);
				}
			}		
		};
    
    protected void onDestroy() 
    { 
//        Log.d(DEBUGTAG,"onDestroy - mDataBaseAdapter:"+mDataBaseAdapter);
        /*******************set last_activity******************/
    	Log.d(DEBUGTAG,"onDestroy");
    	ViroActivityManager.setActivity(null);
    	unregisterReceiver(receiver);
    	releaseAll();    
        super.onDestroy();
    }
    
    @Override
    public void onPause()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onPause");
   /*     if(mQuickAction != null )
        {
            mQuickAction.dismiss();
            mQuickAction = null;
        }*/
        //setListAdapter(null);
        super.onPause();
        overridePendingTransition(0,0);
    }
    
    @Override
    public void onResume()
    {
        super.onResume();
		Log.d(DEBUGTAG,"onResume");
        QtalkSettings.nurse_station_tap_selected = 2;
        Hack.startNursingApp(this);
        updateCallHistory();
        ViroActivityManager.setActivity(this);
//        Log.d(DEBUGTAG,"onResume - mDataBaseAdapter:"+mDataBaseAdapter);
//        if(mDataBaseAdapter == null)
//        {
//            mDataBaseAdapter = new HistoryDataBaseUtility(this);
//            try
//            {
//                mDataBaseAdapter.open(false);
//            }
//            catch(SQLException err)
//            {
//                // data base not exists..
//                Log.e(DEBUGTAG,"data base not available", err);
//            }
//        }  
        if(mDataBaseAdapter != null)
        {
/*            try
            {
                mCallLogList = mDataBaseAdapter.getAllData();
            }
            catch( Exception err)
            {
                Log.e(DEBUGTAG,"DirectoryActivity - ", err);
            }*/
            if(mCallLogList != null)
            {
                ContactListAdapter mContactListAdapter = new ContactListAdapter(this);
                setListAdapter(mContactListAdapter);
            }
            else
            {
            	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"cannot get mCallLogList:"+mCallLogList);
                Toast.makeText(HistoryActivity.this,"Cannot get Directory data", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState); 
        Log.d(DEBUGTAG," onCreate");
        overridePendingTransition(0, 0);
        //---------------------------------------------------------------------[PT]

		IntentFilter intentfilter = new IntentFilter();
		intentfilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
		intentfilter.addAction(LauncherReceiver.PHONE_NEW_NOTIFICATION);
        registerReceiver(receiver, intentfilter);  
		Intent intent = new Intent();
        intent.setAction(LauncherReceiver.PHONE_QUERY_NOTIFICATION);
        sendBroadcast(intent);
        mHandler.postDelayed(CheckNotificationCB, 5000);
        //---------------------------------------------------------------------[PT]
        qtalksetting = QtalkUtility.getSettings(this, engine);
        HistoryDataBaseUtility.setCallBack(HistoryActivity.this, mHisDBHandler);
    	    
    }//oncreate
    
    @Override
    public boolean onSearchRequested() {
        Bundle appDataBundle = new Bundle();
        appDataBundle.putString("search", "");        
        startSearch("", false, appDataBundle, false);//first one is the default search value
        return true;
    }
    
    private class ContactListAdapter extends BaseAdapter 
    {
        public ContactListAdapter(Context context) 
        {
//            Log.d(DEBUGTAG,"ContactListAdapter");
            mContext = context;
        }

        public int getCount() {
            return mCallLogList.size();
//                return mNames.length;
        }

        public Object getItem(int position) {
            return position;
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            CallLogView cv = null;
            //Log.d(DEBUGTAG,"convertView"+convertView +", position:"+position);

            cv = new CallLogView(mContext,
                    mCallLogList.get(position).getQTAccount(),
					mCallLogList.get(position).getName(),
					mCallLogList.get(position).getAlias(),
                    mCallLogList.get(position).getTitle(),
                    mCallLogList.get(position).getRole(),
                    mCallLogList.get(position).getUid(),
                    mCallLogList.get(position).getCallType(),
                    mCallLogList.get(position).getDate(),
                    mCallLogList.get(position).getDuring(),
                    mCallLogList.get(position).getMedia(),
                    mCallLogList.get(position).getKeyId(),
            		mCallLogList.get(position).getIncomingNum());

            if(screenSize>=4){
            	//cv.setBackgroundResource(R.drawable.nss_card_history);
            }else{
            	if(position%2==0){
            		cv.setBackgroundResource(R.drawable.selector_btn_evenhistory);
            	}else{
            		cv.setBackgroundResource(R.drawable.selector_btn_oddhistory);
            	}
            }
            return cv;
               
        }


     }//END of class ContactListAdapter
     private class CallLogView extends LinearLayout 
     {
            @SuppressLint("NewApi")
			public CallLogView(final Context context,final String number, final String name, final String alias, final String title, final String role, final String uid
            		, final int type, final String date, final String during, final int media, final int dataId, final int IncomingNum) 
            {
            	
                super(context);
                Log.d(DEBUGTAG,"number: "+number+" name: "+name+" alias: "+alias+" title: "+title+" role: "+role+" uid: "+uid+" type: "+type+" date: "+date+" during: "+during+
                		" media: "+media+" dataId: "+dataId+" IncomingNum: "+IncomingNum);
                this.setOrientation(HORIZONTAL);
                
                //this.setPadding(screenwidth/7, 0, screenwidth/7, 0);
                boolean is_in_phonebook = true;
                RelativeLayout buttom = new RelativeLayout(context);
                LinearLayout.LayoutParams buttom_para;
                LinearLayout.LayoutParams calltypelayout;
                //LinearLayout buttom = new LinearLayout(context);
				if(QtalkSettings.ScreenSize == -1) {
					Configuration config = getResources().getConfiguration();
					QtalkSettings.ScreenSize = (config.screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK);
				}
                if(QtalkSettings.ScreenSize>=4){
                	this.setPadding(screenwidth/7, 0, screenwidth/7, 0);
	                buttom_para = new LinearLayout.LayoutParams(
	                		LayoutParams.WRAP_CONTENT,LayoutParams.MATCH_PARENT);
	                calltypelayout = new LinearLayout.LayoutParams(
	                		LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);

	                buttom.setPadding(0, screenheight/54, 0, 0);
	                buttom.setBackgroundResource(R.drawable.selector_nss_history);
                }else{
                	this.setPadding(0, 25, 0, 25);
                	
	                buttom_para = new LinearLayout.LayoutParams(
		             	   LayoutParams.MATCH_PARENT,LayoutParams.WRAP_CONTENT);  
	                calltypelayout = new LinearLayout.LayoutParams(
	                	   LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);
	                calltypelayout.gravity = Gravity.CENTER_VERTICAL;
                }
                addView(buttom, buttom_para);
               // this.setPadding(0, 25, 0, 0);
                //Call type pic============================================
                mCallTypePic = new ImageView(context);
                mCallType = new TextView(context); 
               
                //addView(mCallTypePic,calltypelayout);
                if (type == 1)
                {
                	mCallType.setTextAppearance(context, R.style.callTypeNormal);
                	if(role.contentEquals("tts")||role.contentEquals("mcu")){
                		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_incoming_broadcast));
                		if(role.contentEquals("tts")){
                			mCallType.setText(getString(R.string.received_tts));
                		}else{
                			mCallType.setText(getString(R.string.received_ptt));
                		}
                	}else{
                		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_incoming));
                		mCallType.setText(getString(R.string.received_incoming));
                	}
                }
                else if (type == 2)
	            {
                	mCallType.setTextAppearance(context, R.style.callTypeNormal);
                	if(role.contentEquals("tts")||role.contentEquals("mcu")){
	            		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_outgoing_broadcast));
                		if(role.contentEquals("tts")){
                			mCallType.setText(getString(R.string.sent_tts));
                		}else{
                			mCallType.setText(getString(R.string.sent_ptt));
                		}
                	}else{
	            		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_outgoing));
	            		mCallType.setText(getString(R.string.made_outgoing));
	            	}
                }
                else
                {
                	mCallType.setTextAppearance(context, R.style.callTypeFail);
                	if(role.contentEquals("mcu")){
                		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_miss_broadcast));
                		mCallType.setText(getString(R.string.missed_tts));
                	}else{
                		mCallTypePic.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_call_misscall));
                		mCallType.setText(getString(R.string.missed_call));
                	}
                }
                
                //Contact pic============================================
                mContactPhoto = new ImageView(context);
                //LinearLayout.LayoutParams contactphotolayout = new LinearLayout.LayoutParams(
                //		photoSize, photoSize);
                LinearLayout.LayoutParams contactphotolayout = new LinearLayout.LayoutParams(
                		LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                

              /*  if (mContactPhoto != null) {
                    setPhoto(type, media);
                }*/
                
                //Log.d(DEBUGTAG, "==>CallLogView - number: "+number +", name:"+name +", type:"+type +", date:"+date + ", media:"+media);
                
                LinearLayout callTypeCell=new LinearLayout(context);
                callTypeCell.setOrientation(HORIZONTAL);
                callTypeCell.addView(mCallTypePic,calltypelayout);
                
                
                
                
                LinearLayout contactCell=new LinearLayout(context);
                LinearLayout name_number_cell=new LinearLayout(context);
                if(QtalkSettings.ScreenSize==4){
                	if(type==3){
                		mCallType.setBackgroundResource(R.drawable.btn_history_red);
                	}else{
                		mCallType.setBackgroundResource(R.drawable.btn_history_green);
                	}
                	mCallType.setGravity(Gravity.CENTER);
                	mCallTypePic.setScaleType(ScaleType.CENTER_INSIDE);
                	mCallTypePic.setPadding(screenwidth/40, 0, 0, 0);
                	contactCell.setOrientation(HORIZONTAL);  
                	contactphotolayout.setMargins(screenwidth/50, 0, 0, 0);
                	calltypelayout.setMargins(0, 0, 0, 0);
                	calltypelayout.gravity = Gravity.CENTER;
                	
                	if(Hack.isNEC()){
	                	contactCell.addView(callTypeCell,new LinearLayout.LayoutParams(
                                screenwidth/5, (LayoutParams.WRAP_CONTENT)));
                	}else{
                		contactCell.addView(callTypeCell,new LinearLayout.LayoutParams(
                                LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                	}
                    contactCell.addView(mContactPhoto,contactphotolayout);
                    name_number_cell.setGravity(Gravity.CENTER);
                    contactCell.addView(name_number_cell, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)); 
                    if(qtalksetting.provisionType.contentEquals("?type=hh")){
                    	name_number_cell.setPadding(screenwidth/80, 0, 0, 0);
                    	mContactPhoto.setVisibility(View.GONE);
                    }
                	mContactPhoto.setVisibility(View.GONE);
                    callTypeCell.addView(mCallType,calltypelayout);
                }else{
                	//mCallTypePic.setScaleType(ScaleType.FIT_XY);
                	contactCell.setOrientation(VERTICAL);  
                    calltypelayout.setMargins(screenwidth/40, 0, 0, 0);
                    mCallType.setPadding(10, 0, 0, 0);
                    contactCell.addView(name_number_cell, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)); 
                    
                    contactCell.addView(callTypeCell,new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                    contactCell.setGravity(Gravity.CENTER_VERTICAL);
                    callTypeCell.addView(mCallType);
                }
                buttom.addView(contactCell,new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

                
                name_number_cell.setOrientation(VERTICAL);    
                //cellP.setMargins(0,1,0,0);
                
               
                LinearLayout timeCell = new LinearLayout(context);

                RelativeLayout.LayoutParams timeCell_para = new RelativeLayout.LayoutParams(
             		   LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT);

               	DisplayMetrics dm = new DisplayMetrics(); 
        		getWindowManager().getDefaultDisplay().getMetrics(dm);
        		dpi = dm.densityDpi;
        		Configuration config = getResources().getConfiguration();
        		QtalkSettings.ScreenSize = (config.screenLayout&Configuration.SCREENLAYOUT_SIZE_MASK);
        		
                if(QtalkSettings.ScreenSize==4){
                	timeCell.setOrientation(HORIZONTAL);
                	//timeCell.setOrientation(VERTICAL);
                	//timeCell_para.addRule(RelativeLayout.CENTER_VERTICAL);
                	timeCell_para.addRule(RelativeLayout.CENTER_IN_PARENT);
                	timeCell_para.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                }else{
                	timeCell.setOrientation(VERTICAL);
                    timeCell_para.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                    timeCell_para.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
                    timeCell.setPadding(0, 0, 5, 0);
                }             
                
                //timeCell_para.addRule(RelativeLayout.CENTER_IN_PARENT);
                buttom.addView(timeCell,timeCell_para);                
                
                //NAME
                mName = new TextView(context);
                mName.setSingleLine(true);
                //mName.setEllipsize(TruncateAt.START);
                mName.setTextAppearance(context, R.style.contactHistoryName);
                String phonebook_url = Hack.getStoragePath()+"phonebook.xml";
                ArrayList<QTContact2> temp_list;
				try {
					temp_list = QtalkDB.parseProvisionPhonebook(phonebook_url);
					for (int i=0; i< temp_list.size(); i++)
	        		{
						if(number.startsWith(QtalkSettings.MCUPrefix)){ // when mcu , nurse title is null.
							if(temp_list.get(i).getName().contentEquals(name)){
		                		is_in_phonebook = true;
		                		missRole = temp_list.get(i).getRole();
		            			missName = temp_list.get(i).getName();
		            			mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
		            			break;
							}
						}
	                	if (temp_list.get(i).getPhoneNumber().contentEquals(number))
	                	{
//	                		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "The same Phonenumber is "+temp_list.get(i).getPhoneNumber()+" date: "+date);
	                		missRole = temp_list.get(i).getRole();
	            			missName = temp_list.get(i).getName();
	            			missImage = temp_list.get(i).getImage();
	            			missImage = missImage+".jpg";
	            			is_in_phonebook = missName.contentEquals(name);


//	            			if(ProvisionSetupActivity.debugMode) 
//	            				Log.d(DEBUGTAG, "Role: "+missRole+" Name: "+missName+" missImage: "+missImage);
	            			
	            			if(missRole.contentEquals("nurse")){ 
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
		       				}
		       				else if(missRole.contentEquals("station")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_station_online));
		       				}
		       				else if(missRole.contentEquals("mcu")){
		       				    mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
		       				}
		       				else if(missRole.contentEquals("family")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}
		       				else if(missRole.contentEquals("staff")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_cleaner_online));
		       				}
		       				else if(missRole.contentEquals("patient")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}
		       				else{
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}
	            				
	            	//		}
	            			break;
	                	}

	                	if(i == (temp_list.size()-1)){
	                		is_in_phonebook = false;
	                		missRole = role;
	            			missName = name;
	            			missImage = "Unknown";
	            			if(missRole.contentEquals("nurse")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
		       				}
		       				else if(missRole.contentEquals("station")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_station_online));
		       				}
		       				else if(missRole.contentEquals("mcu") || missRole.contains("tts") ){
		       				    mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
		       				}
		       				else if(missRole.contentEquals("family")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}
		       				else if(missRole.contentEquals("staff")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_cleaner_online));
		       				}
		       				else if(missRole.contentEquals("patient")){
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}
		       				else{
		       					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_patient_online));
		       				}	                	}
	                	
	        		}
/*					if( number.startsWith("MCU")){
                		missRole = "mcu";
            			missImage = "Unknown";
            			mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_nurse_online));
					}*/
				} catch (ParserConfigurationException e) {
					// TODO Auto-generated catch block
					Log.e(DEBUGTAG,"parseProvisionPhonebook",e);
				} catch (SAXException e) {
					Log.e(DEBUGTAG,"parseProvisionPhonebook",e);
					// TODO Auto-generated catch block
				} catch (IOException e) {
					Log.e(DEBUGTAG,"parseProvisionPhonebook",e);
					// TODO Auto-generated catch block
					Message msg1 = new Message();
		            msg1.what = 0;
		            Bundle bundle = new Bundle();
		            bundle.putString(ProvisionSetupActivity.MESSAGE, getString(R.string.loading_phonebook_fail)+"\r\n"+e.toString());
		            msg1.setData(bundle);
		            Handler mHandler = new Handler();
					mHandler.sendMessage(msg1);
				}

				//------------------------------------------------------------- for role job name
				LinearLayout allName =new LinearLayout(context);
				allName.setOrientation(HORIZONTAL);
				if(qtalksetting.provisionType.contentEquals("?type=hh")){
					allName.setPadding(0, screenheight/72, 0, 0);
				}else{
					allName.setPadding(screenwidth/50, screenheight/72, 0, 0);
				}
				mRole = new TextView(context);
                mRole.setSingleLine(true);
                mRole.setEllipsize(TruncateAt.START);
                mRole.setTextAppearance(context, R.style.contactHistoryRoleName);
                String name_phrase=null;
                String[] name_split=null;

                if(title!=null){
                    if(name == null){
                        name_phrase = "";
                    }else if (name.length() > 9){
                        name_phrase = name.substring(0, 9) + "...";
                    }else{
                        name_phrase = name;
                    }
                	if(!title.contentEquals("")){
                		if(title.contentEquals("head_nurse")){
                			missRole_UI = getString(R.string.head_nurse);
                		}else if(title.contentEquals("clerk")) {
                			missRole_UI = getString(R.string.secreatary);
                		}else{
                    		missRole_UI = title;
                		}
                	}else{
                		if(missRole.contentEquals("nurse")){
                			missRole_UI = getString(R.string.nurse);
                		}else if(missRole.contentEquals("station")){
                			missRole_UI = getString(R.string.nurse_station);
                		}else if(missRole.contentEquals("staff")){
                			missRole_UI = getString(R.string.cleaner);
                		}else{
                			missRole_UI = "";
                		}
                	}
                	if(missRole.contentEquals("patient")){
	        			if(!is_in_phonebook){
	        				missRole_UI = "";
	        				name_phrase = getString(R.string.leave_hospital);
	        			}
						else if(qtalksetting.provisionType.contentEquals("?type=ns")
						   && (alias != null) && (alias.trim().length()>=3)) {
							name_phrase = alias;
						}

					}
                }else{
        	        if(name!=null){
        	        	if(missRole.contentEquals("nurse")){
        	        		missRole_UI = getString(R.string.nurse);
        	        	}else if(missRole.contentEquals("station")){
        	        		missRole_UI = getString(R.string.nurse_station);
        	        	}else if(missRole.contentEquals("staff")){
        	        		missRole_UI = getString(R.string.cleaner);
        	        	}else{
        	        		missRole_UI = "";
        	        	}
                        if (name.length() > 9){
                            name_phrase = name.substring(0, 9) + "...";
                        }else{
                            name_phrase = name;
                        }
        	        }else{
        	        	missRole_UI = "";
        	        }
                }
				
				mRole.setText(missRole_UI);
				if(missRole_UI.contentEquals(getString(R.string.secreatary))){
   					mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.icn_secretary_online));
				}
            	if(Hack.isNEC()&&!(getString(R.string.language).contentEquals("中文"))){
            		mRole.setVisibility(View.GONE);
            	}
				allName.addView(mRole,new LinearLayout.LayoutParams(
                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
				allName.addView(mName,new LinearLayout.LayoutParams(
                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));				
				if(role.contentEquals("tts")){
					mName.setMaxEms(8);
					ImageView readmore = new ImageView(context);
					readmore.setBackgroundResource(R.drawable.icn_read_more);
					LinearLayout.LayoutParams readmorepara = new LinearLayout.LayoutParams(
	                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
					readmorepara.gravity = Gravity.CENTER_VERTICAL;
					readmorepara.setMargins(20, 0, 0, 0);
					allName.addView(readmore,readmorepara);	
				}
				
				if (IncomingNum ==0)
				{
					mName.setText(name_phrase);
				}
				else
				{
					mName.setText(name_phrase+" ("+IncomingNum+")");
				}
                    mName.setSingleLine(true);
                    
    				if(missRole_UI.contentEquals("")){
    					mRole.setVisibility(View.GONE);
    					//mName.setPadding(screenwidth/20, 0, 0, 0);
    				}else{
    					if(QtalkSettings.ScreenSize==4){
    						mName.setPadding(screenwidth/100, 0, 0, 0);
    					}else{
    						mName.setPadding(screenwidth/50, 0, 0, 0);
    					}
    				}
                    name_number_cell.addView(allName, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                    if(!is_in_phonebook && !(missRole.contentEquals("mcu")) && !(missRole.contentEquals("tts")) && !(missRole.contentEquals("cmuh"))){
                    	//mName.setAlpha((float) 0.5);
                    	//mRole.setAlpha((float) 0.5);
                    	this.setAlpha((float) 0.5);
                    }
                //Call Type
                mType = new TextView(context);
                mType.setTextAppearance(context, R.style.contactQT);
                //Call During
                mDuring = new TextView(context);
                mDuring.setTextAppearance(context, R.style.contactQTTime);
                mDuring.setSingleLine(true);
                mDuring.setText(during);
                //Call Date
                mDate = new TextView(context);
                mDate.setTextAppearance(context, R.style.contactQTTime);
                mDate.setText(date);
                //mDate.setPadding(50, 0, 0, 0);
                //contactCell.addView(mDate, new LinearLayout.LayoutParams(
                  //      LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                if(QtalkSettings.ScreenSize>=4){
                	//String during_large = "通話時間 "+during;
                	//mDuring.setText(during_large);
	                timeCell.addView(mDuring, new LinearLayout.LayoutParams(
	                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));  
	                mDuring.setGravity(Gravity.LEFT);
	                mDuring.setPadding(0, screenheight/216, screenwidth/40, 0);
	                mDate.setPadding(0, screenheight/216, screenwidth/40, 0);
	                timeCell.addView(mDate, new LinearLayout.LayoutParams(
	                             LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)); 
                }else{
                    timeCell.setGravity(Gravity.RIGHT);
                    timeCell.addView(mDuring, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));   
                    timeCell.addView(mDate, new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)); 	
                }

            }//end of CallLogView constructor 

            private TextView mName;
            private TextView mRole;
            private TextView mType;
            private TextView mDate;
            private TextView mDuring;
            private TextView mCallType;
        //    private ImageView mContactPhoto;
        }//END of CallLogView
     
     @Override
     protected void onListItemClick(ListView l, final View v, int position, final long id)
     {                 
         final CallHistoryInfo call_log_info = mCallLogList.get(position);
         if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"QT name  : " +call_log_info.getQTAccount() );
         String phonebook_url = Hack.getStoragePath()+"phonebook.xml";
         ArrayList<QTContact2> temp_list;
			
				try {
					temp_list = QtalkDB.parseProvisionPhonebook(phonebook_url);
				/*if(QtalkSettings.type.contentEquals("?type=hh")){   //if patient , he/she always call back to main(first)
					String Role = null;                             //nurse on history
					for (int i=0; i< temp_list.size(); i++)
	        		{	
						if (temp_list.get(i).getPhoneNumber().contentEquals(call_log_info.getQTAccount()))
	                	{
							Role = temp_list.get(i).getRole();
							break;
	                	}
	        		}
					for (int i=0; i< temp_list.size(); i++)
	        		{	
						if(Role.contentEquals(TAG_NURSE)){
							if(temp_list.get(i).getRole().contentEquals(Role)){
								makeCall(temp_list.get(i).getPhoneNumber(),false);
								break;
							}
						}
						if (temp_list.get(i).getPhoneNumber().contentEquals(call_log_info.getQTAccount()))
	                	{
							makeCall(call_log_info.getQTAccount(),false);
							break;
	                	}
	        		}
				
				}else{*/
					for (int i=0; i< temp_list.size(); i++)
	        		{	
						if (temp_list.get(i).getPhoneNumber().contentEquals(call_log_info.getQTAccount()))
						{
							if(!(temp_list.get(i).getRole().contentEquals(TAG_BROADCAST))){
				        		makeCall(call_log_info.getQTAccount(),true);
							}
							break;
	                	}
	        		}
				//}
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
         //makeCall(call_log_info.getQTAccount(),true);//String callerID,boolean enableVideo
         
         //finish();
		if (call_log_info.getRole().contentEquals("tts")) {
			if(tts_readmore_popout==null){
				AlertDialog.Builder builder = new AlertDialog.Builder(
						HistoryActivity.this);
				tts_readmore_popout = builder.setTitle(R.string.app_name)
						.setMessage("waiting").setCancelable(true).create();
			}
			if(!tts_readmore_popout.isShowing()){
				tts_readmore_popout.show();
	
				Window win = tts_readmore_popout.getWindow();
				win.setContentView(R.layout.layout_broadcast_readmore_popout);
				TextView tts_message = (TextView) win
						.findViewById(R.id.tts_message);
				tts_message.setText(call_log_info.getName());
				close_btn = (Button) win.findViewById(R.id.btn_close);
				close_btn.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						//tts_readmore_popout.cancel();
						tts_readmore_popout.dismiss();
					}
				});
			}
		}

     }//END of onListItemClick
     
     
  
    @Override
    public void updateCallHistory()
    {
    	Log.d(DEBUGTAG,"updateCallHistory");
    	if(mDataBaseAdapter == null)
        {
            mDataBaseAdapter = new HistoryDataBaseUtility(this);
            try
            {
                mDataBaseAdapter.open(false);
            }
            catch(SQLException err)
            {
                // data base not exists..
                Log.e(DEBUGTAG,"data base not available", err);
            }
        }
        final DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        Configuration config = getResources().getConfiguration();
        screenwidth = (int) dm.widthPixels;
        screenheight = (int) dm.heightPixels;
        screenSize = (config.screenLayout&Configuration.SCREENLAYOUT_SIZE_MASK);
        picSize = screenwidth/10;
        if(screenSize>=4){
            photoSize = screenwidth/15;
            picSize = screenwidth/25;
        }else{
            photoSize = screenwidth/6;
            picSize = screenwidth/10;
        }

        mCallLogList = mDataBaseAdapter.getAllData();
        CallSize = HistoryDataBaseUtility.size;
//        if (CallSize > 1)
//        {
			if(qtalksetting.provisionType.contentEquals("?type=hh")){
				setContentView(R.layout.layout_call_log_pt);
			}else{
				setContentView(R.layout.layout_call_log);
			}
        	
        	Careplan=(ImageButton) findViewById(R.id.CarePlan);
    		Canlendar=(ImageButton) findViewById(R.id.Canlendar);
    		Profile=(ImageButton) findViewById(R.id.Profile);
        	OnClickListener theclick=new OnClickListener(){
                @Override
                public void onClick(View v) {
                    synchronized(HistoryActivity.this)
                    {
						if(v.getId() == R.id.call_contacts){
							Intent contacts = new Intent();
							contacts.setClass(HistoryActivity.this, ContactsActivity.class);
							contacts.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
							startActivity(contacts);
							finish();
							overridePendingTransition(0, 0);
						}
                    }
                }//End of onclick
    	    };
    	    Careplan.setOnClickListener(theclick);
    	    Canlendar.setOnClickListener(theclick);
    	    Profile.setOnClickListener(theclick);
      		Contacts = (Button) findViewById(R.id.call_contacts);
      		Contacts.setOnClickListener(theclick);
      		
      		
      		Contact = (Button) findViewById(R.id.bottom_contacts);
        	Contact.setOnClickListener(new OnClickListener() {
				
				@Override
				public void onClick(View v) {
					if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"open contacts");
					Intent contact = new Intent();
					contact.setClass(HistoryActivity.this, ContactsActivity.class);
      	  			startActivity(contact);
					finish();
					overridePendingTransition(0, 0);
					QtalkSettings.nurse_station_tap_selected = 1;
				}
			});

        	if(screenSize>=4){
        		Contact_info = (Button) findViewById(R.id.contacts_info);
        		Contact_info.setOnClickListener(new OnClickListener() {
					
					@Override
					public void onClick(View v) {
	                	 	Log.d(DEBUGTAG,"send open contacts_info");
  	                	Intent patient_info = new Intent();
  	                	patient_info.setAction("android.intent.action.qlauncher.patient");
	        	  			sendBroadcast(patient_info);
	        	  			finish();
	        	  			overridePendingTransition(0, 0);
							QtalkSettings.nurse_station_tap_selected = 0;
					}
				});
            	if(qtalksetting.provisionType.contentEquals("?type=hh")){
            			Contact_info.setVisibility(View.INVISIBLE);
            	}
            	
        	}
    }
    
    private void releaseAll()
    {
    	/*if (mDataBaseAdapter!=null)
        	mDataBaseAdapter.clearData();
        if(mCallLogList != null)
            mCallLogList.clear();
        setListAdapter(null);*/
        //mDataBaseAdapter.close();
        missImage=null;
        missName=null;
        missRole=null;
        if (Temp1 != null)
        {
        	Temp1.setCallback(null);
        	Temp1=null;
        }
 //      mCursor=null;
        if (mContactPhoto != null)
        {
        	if( mContactPhoto.getDrawable()!=null){
        		mContactPhoto.getDrawable().setCallback(null);
        	}
        	mContactPhoto = null;
        }
        mContactListAdapter=null;
        mHandler.removeCallbacks(CheckNotificationCB);
        cv=null; 
        mContext=null;
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE); 
        notificationManager.cancel(QTReceiver.NOTIFICATION_ID_CALLLOG);
        Careplan = null;
        Canlendar = null;
        Profile = null;
    	Contacts = null;
    }
    
	@Override  
    public boolean onTouchEvent(MotionEvent event) {
	    return super.onTouchEvent(event);
    }  
	
    @Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		 if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_HOME) {
			 finish();
			 overridePendingTransition(0, 0);
		 }
		return super.onKeyDown(keyCode, event);
	}
    
}
