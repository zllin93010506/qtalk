package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.text.TextUtils.TruncateAt;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.BaseExpandableListAdapter;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnGroupClickListener;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkPreference;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import javax.xml.parsers.ParserConfigurationException;

@SuppressLint("NewApi")
public class ContactsActivity extends AbstractContactsActivity {

    private static final String DEBUGTAG = "QTFa_ContactsActivity";

    Activity MyActivity = this;
    public static int dpi = 0, ScreenSize = 0;
    private final String phonebook_url = Hack.getStoragePath() + "phonebook.xml";
    private final String ttslist_url = Hack.getStoragePath() + "ttsList";
    boolean state = false;
    private ImageView mContactPhoto;
    int photoSize = 180;
    ImageButton Careplan = null, Canlendar = null, Profile = null;
    ImageButton Notification_Unread = null, Notification_None = null;
    Button Keypad = null, CallHistory = null;
    Button Contact_info = null;
    RelativeLayout layout_contactlist2 = null;
    LinearLayout first = null;
    LinearLayout ll_large = null;
    //	int PatientNumber = 0, FamilyNumber = 0;
    Button History = null;
    Drawable Temp1;
    ArrayList<QTContact2> temp_list;
    ContactListAdapter mContactListAdapter;
    ContactExpandListAdapter mContactExpandListAdapter;
    ContactsView cv = null;
    QtalkEngine engine;
    QtalkSettings qtalksetting;
    String current_role = "un-known";
    public int screenwidth = 0;
    public int screenheight = 0;
    public int default_pt_member = 6;
    private int xlarge_contacts_gap = 150;

    private Context mContext;
    public boolean first_nurse = true;
    public int real_position = 0;

    private boolean newNotification = false;

    Button user_info, collapse;
    String sip_prefix = "5912";
    final int xlarge_contactcount_row = 3;
    ExpandableListView elv = null;

    ExecutorService pool = Executors.newFixedThreadPool(2);

    private BroadcastReceiver receiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(DEBUGTAG, "action:" + intent.getAction());
            if (intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                //finish();
                //overridePendingTransition(0, 0);
                Log.d(DEBUGTAG, "reveive ACTION_CLOSE_SYSTEM_DIALOGS");
            } else if (intent.getAction().equals(LauncherReceiver.PHONE_NEW_NOTIFICATION)) {
                newNotification = true;
                Bundle bundle = intent.getExtras();
                int count = bundle.getInt("NEW_ITEM");
                if (count > 0) {
                    //Notification.setBackground(getResources().getDrawable(R.drawable.btn_notification_new_call));
                    Log.d(DEBUGTAG, "there is new notifycation");
                } else {
                    Log.d(DEBUGTAG, "there is no new notifycation");
                }
                mHandler.removeCallbacks(CheckNotificationCB);
            }
        }
    };

    @Override
    public void onPause() {
        super.onPause();
        overridePendingTransition(0, 0);
        if (Hack.isPhone())
            finish();
    }

    @Override
    public void onStop() {
        super.onStop();
        MyActivity = null;
    }

    @Override
    public void onStart() {
        super.onStart();
        Log.d(DEBUGTAG, "onStart");
    }

    private Handler mHandler = new Handler();

    final Runnable CheckNotificationCB = new Runnable() {
        @Override
        public void run() {
            if (!newNotification) {
                Intent intent = new Intent();
                intent.setAction(LauncherReceiver.PHONE_QUERY_NOTIFICATION);
                sendBroadcast(intent);
                mHandler.postDelayed(CheckNotificationCB, 5000);
            }
        }
    };

    final Runnable FinishThread = new Runnable() {
        public void run() {
            finish();
            overridePendingTransition(0, 0);
        }
    };
    final Runnable AvatarUpdateCheck = new Runnable() {
        public void run() {
            if (Flag.Phonebook.getState()) {
                updateCallAvatar();

                Flag.Phonebook.setState(false);
                //------------------------------------------------------------[PT]
                Intent broadcast_intent = new Intent();
                broadcast_intent.setAction("android.intent.action.vc.phonebookupdate");
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "Send Broadcast to phonebookupdate SUCCESS");
                sendBroadcast(broadcast_intent);
                // ------------------------------------------------------------[PT]
                File TTSListFile = new File(ttslist_url);
                if (!TTSListFile.exists()) {
                    Intent intent = new Intent();
                    intent.setAction(LauncherReceiver.ACTION_GET_TTS);
                    sendBroadcast(intent);
                }
            }
            //mHandler.postDelayed(AvatarUpdateCheck, 10000);
            mHandler.postDelayed(AvatarUpdateCheck, 5000);
        }
    };

    private void setPhonebook() {
        CallHistory.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "open call history");
                Intent history = new Intent();
                history.setClass(ContactsActivity.this, HistoryActivity.class);
                history.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                startActivity(history);
                finish();
                overridePendingTransition(0, 0);
                QtalkSettings.nurse_station_tap_selected = 2;
            }
        });

        Contact_info = (Button) findViewById(R.id.contacts_info);
        Contact_info.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                Log.d(DEBUGTAG, "send open contacts_info");
                Intent patient_info = new Intent();
                patient_info.setAction("android.intent.action.qlauncher.patient");
                sendBroadcast(patient_info);
                finish();
                overridePendingTransition(0, 0);
                QtalkSettings.nurse_station_tap_selected = 0;

            }
        });
        if (qtalksetting.provisionType.contentEquals("?type=hh")) {
            Contact_info.setVisibility(View.INVISIBLE);
        }
        Notification_Unread = (ImageButton) findViewById(R.id.notification_unread);
        Notification_None = (ImageButton) findViewById(R.id.notification_none);
        if (Flag.Notification.getState()) {
            Notification_Unread.setVisibility(View.VISIBLE);
            Notification_None.setVisibility(View.GONE);
        } else {
            Notification_Unread.setVisibility(View.GONE);
            Notification_None.setVisibility(View.VISIBLE);
        }
        OnClickListener theclick = new OnClickListener() {
            @Override
            public void onClick(View v) {
                synchronized (ContactsActivity.this) {
                    int id = v.getId();
                    if (id == R.id.call_history) {
                        Log.d(DEBUGTAG, "send open call_history");
                        Intent history = new Intent();
                        history.setClass(ContactsActivity.this, HistoryActivity.class);
                        history.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                        startActivity(history);
                        finish();
                        overridePendingTransition(0, 0);
                    } else if (id == R.id.notification_none) {
                        Intent notification_none = new Intent();
                        notification_none.setClass(ContactsActivity.this, NotificationActivity.class);
                        startActivity(notification_none);
                    } else if (id == R.id.notification_unread) {
                        Intent notification_unread = new Intent();
                        notification_unread.setClass(ContactsActivity.this, NotificationActivity.class);
                        startActivity(notification_unread);
                    }
                }
            }//End of onclick
        };
        Canlendar = (ImageButton) findViewById(R.id.Canlendar);
        Canlendar.setOnClickListener(theclick);
        Careplan = (ImageButton) findViewById(R.id.CarePlan);
        Careplan.setOnClickListener(theclick);
        Profile = (ImageButton) findViewById(R.id.Profile);
        Profile.setOnClickListener(theclick);
        History = (Button) findViewById(R.id.call_history);
        History.setOnClickListener(theclick);
        Notification_Unread.setOnClickListener(theclick);
        Notification_None.setOnClickListener(theclick);
        //Read phonebook.xml
        File PhonebookFile = new File(phonebook_url);
        if (PhonebookFile.exists()) {
            try {
                temp_list = QtalkDB.parseProvisionPhonebook(phonebook_url);
            } catch (ParserConfigurationException e) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e);
            } catch (SAXException e) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e);
            } catch (IOException e) {
                // TODO Auto-generated catch block
                Log.e(DEBUGTAG, "", e);
            }
        }

        File TTSListFile = new File(ttslist_url);
        if (!TTSListFile.exists()) {
            Intent intent = new Intent();
            intent.setAction(LauncherReceiver.ACTION_GET_TTS);
            sendBroadcast(intent);
        }

        if (temp_list != null) {
            ExpandableListView elv = (ExpandableListView) findViewById(R.id.mExpandableListView);
            LinearLayout bly = (LinearLayout) findViewById(R.id.mBroadcastList);
            setExpandableListView(elv, bly);
        } else {
            Log.d(DEBUGTAG, "[ERROR]Contacts List is null!!!");
        }
    }//end setphonebook

    void openUserInfoDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(
                ContactsActivity.this);
        final AlertDialog userinfo_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                .setCancelable(true).create();
        userinfo_popout.show();

        Window win = userinfo_popout.getWindow();
        win.setContentView(R.layout.user_info_dialog);
        TextView UserName = (TextView) win.findViewById(R.id.user_name);
        TextView UserNumber = (TextView) win.findViewById(R.id.user_number);
        TextView VersionNumber = (TextView) win.findViewById(R.id.userinfo_version);
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(mContext);
        String sip_number = preference.getString(QtalkPreference.SETTING_SIP_ID, null);
        String display_name = preference.getString(QtalkPreference.SETTING_DISPLAY_NAME, null);

        String user_number = null;
        if (sip_number.startsWith(sip_prefix)) {
            //user_number = getString(R.string.phone_number)+" : "+"("+sip_prefix+")"+sip_number.substring(sip_prefix.length());
            user_number = getString(R.string.phone_number) + " : " + sip_number;
        } else {
            user_number = sip_number;
        }
        String version = null;
        int versionCode = 0;
        try {
            version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
            versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
            VersionNumber.setText("QSPT-VC " + version);
            //VersionNumber.setText("QSPT-VC "+version+" ("+versionCode+") ");
        } catch (NameNotFoundException e1) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "getVersion", e1);
            Log.e(DEBUGTAG, "", e1);
        }
    		
    		/*VersionNumber.setOnClickListener(new OnClickListener() {
				
				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
					Intent loop = new Intent();
                    loop.setClass(ContactsActivity.this, InVideoSessionActivity.class);
        	  		startActivity(loop);
        	  		finish();
				}
			});*/

        UserName.setText(display_name);
        UserNumber.setText(user_number);

        Button btn_close = (Button) win.findViewById(R.id.btn_close);
        btn_close.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                userinfo_popout.cancel();
            }
        });

    }


    //-------------------------------------------------------------------------------[PT]
    private class ContactExpandListAdapter extends BaseExpandableListAdapter {
        private Context context;
        List<Map<String, String>> groups;
        List<List<Map<String, ContactsView>>> childs;

        public ContactExpandListAdapter(Context context,
                                        List<Map<String, String>> groups,
                                        List<List<Map<String, ContactsView>>> childs) {
            this.groups = groups;
            this.childs = childs;
            this.context = context;
        }

        @Override
        public Object getChild(int groupPosition, int childPosition) {
            return childs.get(groupPosition).get(childPosition);
        }

        @Override
        public long getChildId(int groupPosition, int childPosition) {
            return childPosition;
        }

        @Override
        public View getChildView(int groupPosition, int childPosition,
                                 boolean isLastChild, View convertView, ViewGroup parent) {

            LinearLayout ll_large = new LinearLayout(context);
            LayoutParams ll_para = new LayoutParams(
                    LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);

            for (int i = (childPosition * xlarge_contactcount_row); i < ((childPosition + 1) * xlarge_contactcount_row); i++) {
                if (i < childs.get(groupPosition).size()) {
                    ContactsView cv = ((Map<String, ContactsView>) getChild(groupPosition,
                            i)).get("child");
                    LinearLayout ll_parent = (LinearLayout) cv.getParent();
                    if (ll_parent != null) {
                        ll_parent.removeAllViews();
                    }
                    cv.setGravity(Gravity.CENTER);
                    cv.setPadding(10, 0, 10, 0);
                    LayoutParams cv_para = new LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                    if (Hack.isNEC()) {
                        cv_para.weight = screenwidth / 4;
                        ll_large.addView(cv, cv_para);
                    } else {
                        cv_para.weight = screenwidth / 4;
                        ll_large.addView(cv, cv_para);
                    }
                    ll_large.setGravity(Gravity.CENTER);
                    //((LinearLayout.LayoutParams)cv.getLayoutParams()).width=screenwidth/4;
                } else {
                    LinearLayout empty = new LinearLayout(context);
                    LinearLayout empty_bottom = new LinearLayout(context);
                    empty_bottom.setBackgroundResource(R.drawable.selector_btn_list_contact_xlarge);
                    empty.addView(empty_bottom);
                    LayoutParams empty_para = new LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);

                    empty.setPadding(10, 0, 10, 0);
                    empty.setVisibility(View.INVISIBLE);

                    if (Hack.isNEC()) {
                        empty_para.weight = screenwidth / 4;
                        ll_large.addView(empty, empty_para);
                    } else {
                        empty_para.weight = screenwidth / 4;
                        ll_large.addView(empty, empty_para);
                    }
                }
            }

            int listsize = childs.get(groupPosition).size();
            if (listsize % xlarge_contactcount_row == 0) {
                listsize = (listsize / xlarge_contactcount_row);
            } else {
                listsize = (listsize / xlarge_contactcount_row) + 1;
            }
            if (childPosition == 0) {
                if (childPosition == listsize - 1) {
                    ll_large.setPadding(xlarge_contacts_gap, 30, xlarge_contacts_gap, 30);
                } else {
                    ll_large.setPadding(xlarge_contacts_gap, 30, xlarge_contacts_gap, 5);
                }
            } else if (childPosition == listsize - 1) {
                ll_large.setPadding(xlarge_contacts_gap, 5, xlarge_contacts_gap, 30);
            } else {
                ll_large.setPadding(xlarge_contacts_gap, 5, xlarge_contacts_gap, 5);
            }

            return ll_large;

        }

        public int getChildrenCount(int groupPosition) {
            ScreenSize = 4;
            int listsize = childs.get(groupPosition).size();
            if (listsize % xlarge_contactcount_row == 0) {
                return (listsize / xlarge_contactcount_row);
            } else {
                return (listsize / xlarge_contactcount_row) + 1;
            }
        }

        public Object getGroup(int groupPosition) {
            return groups.get(groupPosition);
        }

        public int getGroupCount() {
            return groups.size();
        }

        public long getGroupId(int groupPosition) {
            return groupPosition;
        }

        // 獲取一級清單View物件
        public View getGroupView(int groupPosition, boolean isExpanded,
                                 View convertView, ViewGroup parent) {


            String text = groups.get(groupPosition).get("group");
            LayoutInflater layoutInflater = (LayoutInflater) context
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);

            // 獲取一級清單佈局檔,設置相應元素屬性
            LinearLayout linearLayout = (LinearLayout) layoutInflater.inflate(
                    R.layout.expandlist_group, null);
            TextView textView = (TextView) linearLayout
                    .findViewById(R.id.group_tv);
            ImageView groupIconPhoto = new ImageView(context);
            textView.setText(text);
            groupIconPhoto = (ImageView) linearLayout.findViewById(R.id.group_icon);
            if (text.contentEquals(getString(R.string.broadcast))) {
                textView.setTextAppearance(context, R.style.contactGroupBroadcast);
                linearLayout.setVisibility(View.GONE);
            } else if (text.contentEquals(getString(R.string.doctor))) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_patient);
            } else if (text.contentEquals(getString(R.string.nurse))) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_nurse);
            } else if (text.contentEquals(getString(R.string.patient))) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_patient);
            } else if (text.contentEquals(getString(R.string.cleaner))) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_cleaner);
            } else if (text.contentEquals(getString(R.string.family))) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_cleaner);
            } else if (text.contentEquals("其他")) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_cleaner);
            } else {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_contact_cleaner);
            }
            if (ScreenSize == 4) {
                textView.setTextAppearance(context, R.style.contactGroupOther);
                linearLayout.setBackgroundResource(R.drawable.img_large_contact);
                textView.setPadding(40, 5, 0, 5);
            } else {
                textView.setPadding(5, 5, 0, 5);
            }
            groupIconPhoto.setVisibility(View.GONE);
            SharedPreferences settings = getSharedPreferences("EXPAND", 0);
            SharedPreferences.Editor PE = settings.edit();
            PE.putBoolean(text, isExpanded);
            PE.commit();

            if (qtalksetting.provisionType.contentEquals("?type=hh")) {
                if (text.contentEquals(getString(R.string.nurse)) ||
                        text.contentEquals(getString(R.string.family)) ||
                        text.contentEquals(getString(R.string.doctor))) {

                } else {
                    linearLayout.setVisibility(View.GONE);
                }
            } else {
                if (text.contentEquals("其他")) {
                    //linearLayout.setVisibility(View.GONE);
                }
            }

            return linearLayout;
        }

        public boolean hasStableIds() {
            return false;
        }

        public boolean isChildSelectable(int groupPosition, int childPosition) {
            return false;
        }

    }
    //-------------------------------------------------------------------------------

    private class ContactListAdapter extends BaseAdapter {
        public ContactListAdapter(Context context) {
            mContext = context;
        }

        public int getCount() {
            if (ScreenSize < 4) {
                return temp_list.size();
            } else {
                int fake_list_number = temp_list.size() - 1;
                if (fake_list_number <= default_pt_member) {
                    return 2;
                } else {
                    if (fake_list_number % 3 == 0) {
                        return (fake_list_number / 3);
                    } else {
                        return (fake_list_number / 3) + 1;
                    }
                }
//                    return mNames.length;
            }
        }

        public Object getItem(int position) {
            return position;
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {

            if (ScreenSize < 4) {
                ContactsView cv = null;
                cv = new ContactsView(mContext,
                        temp_list.get(position).getImage(),
                        temp_list.get(position).getTitle(),
                        temp_list.get(position).getName(),
                        temp_list.get(position).getAlias(),
                        temp_list.get(position).getRole(),
                        temp_list.get(position).getPhoneNumber(),
                        temp_list.get(position).getOnline());
                cv.setBackgroundResource(R.drawable.selector_btn_name);
                return cv;
            } else {
                LinearLayout test = new LinearLayout(mContext);

                for (int i = (position * 3) + 1; i < ((position + 1) * 3) + 1; i++) {
                    if (i < temp_list.size()) {
                        ContactsView cv = null;
                        //Log.d(DEBUGTAG,"convertView"+convertView +", position:"+position);

                        cv = new ContactsView(mContext,
                                temp_list.get(i).getImage(),
                                temp_list.get(i).getTitle(),
                                temp_list.get(i).getName(),
                                temp_list.get(i).getAlias(),
                                temp_list.get(i).getRole(),
                                temp_list.get(i).getPhoneNumber(),
                                temp_list.get(i).getOnline());
                        final String remoteID = temp_list.get(i).getPhoneNumber();
                        OnClickListener makecallclick = new OnClickListener() {
                            @Override
                            public void onClick(View cv) {
                                Log.d(DEBUGTAG, "1 remoteID: " + remoteID);
                                makeCall(remoteID, true);//String callerID,boolean enableVideo
                            }
                        };
                        cv.setOnClickListener(makecallclick);

                        test.addView(cv);
                        //((LinearLayout.LayoutParams)cv.getLayoutParams()).weight=1.0f;
                        ((LinearLayout.LayoutParams) cv.getLayoutParams()).width = screenwidth / 4;
                    } else if (i <= default_pt_member) {
                        cv = new ContactsView(mContext,
                                "unknown",
                                "unknown",
                                "unknown",
                                "unknown",
                                "unknown",
                                "unknown",
                                "unknown");
                        OnClickListener makecallclick = new OnClickListener() {
                            @Override
                            public void onClick(View cv) {
                            }
                        };
                        cv.setOnClickListener(makecallclick);

                        test.addView(cv);
                        LinearLayout.LayoutParams x = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.FILL_PARENT);

                        //((LinearLayout.LayoutParams)cv.getLayoutParams()).weight=1.0f;
                        ((LinearLayout.LayoutParams) cv.getLayoutParams()).width = screenwidth / 4;
                    }
                }
                return test;
            }
        }
    }//END of class ContactListAdapter

    private class ContactsView extends LinearLayout {
        @SuppressLint("NewApi")
        public ContactsView(final Context context, final String image, final String title, final String name, final String alias, final String role, final String number, String online) {
            super(context);
            LayoutInflater inflater = (LayoutInflater) ContactsActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            inflater.inflate(R.layout.contacts_button, this, true);
            String titleName = null;
            String realName = null;
            if (qtalksetting.provisionType.contentEquals("?type=ns")
                    && (alias != null) && (alias.trim().length() >= 3)) {
                realName = alias;
            } else
                realName = name;
            if (title != null) {
                if (!(title.contentEquals("")))
                    titleName = title;
                if (title.contentEquals("head_nurse")) {
                    titleName = getString(R.string.head_nurse);
                } else if (title.contentEquals("clerk")) {
                    titleName = getString(R.string.secreatary);
                }
            } else {
                if (role.contentEquals("station") && !qtalksetting.provisionType.contentEquals("?type=hh")) {
                    titleName = getString(R.string.nurse_station);
                }
            }
            mName = (TextView) findViewById(R.id.contact_name);
            mNameTitle = (TextView) findViewById(R.id.contact_title);
            Contacts = (LinearLayout) findViewById(R.id.contact_bottom);
            mContactPhoto = (ImageView) findViewById(R.id.contactsphoto);


            //NAME
            mName.setSingleLine(true);
            mName.setEms(9);
            mName.setEllipsize(TruncateAt.END);
            //TITLE
            mNameTitle.setSingleLine(true);
            mNameTitle.setEllipsize(TruncateAt.END);


            mNameTitle.setText(titleName);

            if (role.contentEquals("family") || role.contentEquals("patient") || role.contentEquals("nurse")
                    || role.contentEquals("station") || role.contentEquals("mcu") || role.contentEquals("unknown") ||
                    role.contentEquals("staff") || role.contentEquals("virtual") || role.contentEquals("doctor")) {

                //Contact pic============================================
                RelativeLayout.LayoutParams contactphotolayout = new RelativeLayout.LayoutParams(
                        LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                File file_temp = new File(image);
	                  /* if (file_temp.exists()&& !role.contentEquals("station"))
	       				{	
	                	    file_temp = new File (image+".jpg");
	                	    if (!file_temp.exists() || Flag.Phonebook.getState())
	                	    {
	                	    	try {
									DrawableTransfer.DrawImageConer(image);
								} catch (IOException e) {
									// TODO Auto-generated catch block
									Log.e(DEBUGTAG,"",e);
								}
	                	    }
	                	    Temp1 = DrawableTransfer.LoadImageFromWebOperations(image+".jpg");
		       				mContactPhoto.setImageDrawable(Temp1);
	       				}
		       			else
		       			{*/
                if (role.contentEquals("nurse")) {
                    if (title.contentEquals("clerk")) {
                        if (online.contentEquals("1")) {
                            mContactPhoto.setBackgroundResource(R.drawable.icn_secretary_online);
                        } else {
                            mContactPhoto.setBackgroundResource(R.drawable.icn_secretary_offline);
                        }
                    } else {
                        if (online.contentEquals("1")) {
                            mContactPhoto.setBackgroundResource(R.drawable.icn_nurse_online);
                        } else {
                            mContactPhoto.setBackgroundResource(R.drawable.icn_nurse_offline);
                        }
                    }
                } else if (role.contentEquals("station")) {
                    if (online.contentEquals("1")) {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_nurse_station);
                    } else {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_nurse_station);
                    }

                } else if (role.contentEquals("mcu")) {
                    if (ScreenSize >= 4) {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_broadcast);
                    } else {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_broadcast);
                    }
                } else if (role.contentEquals("family")) {
                    if (online.contentEquals("1")) {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_patient_online);
                    } else {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_patient_offline);
                    }
                } else if (role.contentEquals("patient")) {
                    if (online.contentEquals("1")) {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_patient_online);
                    } else {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_patient_offline);
                    }
                } else if (role.contentEquals("staff")) {
                    if (online.contentEquals("1")) {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_cleaner_online);
                    } else {
                        mContactPhoto.setBackgroundResource(R.drawable.icn_cleaner_offline);
                    }
                } else {
                    mContactPhoto.setImageDrawable(this.getResources().getDrawable(R.drawable.all_icn_unknown));
                }

                if (ScreenSize < 4) {
                    if (role.contentEquals("station")
                            || role.contentEquals("mcu")) {
                        mContactPhoto.setVisibility(View.VISIBLE);
                    } else {
                        mContactPhoto.setVisibility(View.GONE);
                    }

                    if (role.contentEquals("mcu")) {
                        Contacts.setBackgroundResource(R.drawable.selector_btn_list_broadcast);
                    } else if (role.contentEquals("nurse")
                            || role.contentEquals("station")) {
                        Contacts.setBackgroundResource(R.drawable.selector_btn_list_nurse);
                    } else if (role.contentEquals("patient")) {
                        Contacts.setBackgroundResource(R.drawable.selector_btn_list_patient);
                    } else if (role.contentEquals("staff")) {
                        Contacts.setBackgroundResource(R.drawable.selector_btn_list_staff);
                    }

                } else {
                    if (!role.contentEquals("mcu")) {
                        mContactPhoto.setVisibility(View.GONE);
                    }
                    Contacts.setBackgroundResource(R.drawable.selector_btn_list_contact_xlarge);
                }

                if (online.contentEquals("1")) {      // online offline and station name adjust
                    if (role.contentEquals("station")) {
                        if (qtalksetting.provisionType.contains("?type=hh")) {
                            mName.setText(getString(R.string.nurse_station));
                        } else {
                            String name_station = realName;
                            mName.setText(name_station);
                        }

                    } else {
                        //String test_name = name.replace(" ", )
                        mName.setText(realName);
                    }
                } else {
                    String name_offline = null;
                    if (role.contentEquals("station")) {
                        if (qtalksetting.provisionType.contains("?type=hh")) {
                            mName.setText(getString(R.string.nurse_station));
                        } else {
                            String name_station = realName;
                            mName.setText(name_station);
                        }
                    } else {
                        if (ScreenSize < 4) {
                            name_offline = realName;
                            //   mName_offine.setText("");
                            //mName_offine.setText(" ("+getString(R.string.offline)+")");
                        } else {
                            name_offline = realName;
                            //	   mName_offine.setText("");
                        }
                        mName.setText(name_offline);
                    }
                    if (!role.contentEquals("doctor")) {
                        mName.setAlpha((float) 0.5);
                        //mName_offine.setAlpha((float) 0.5);
                        mNameTitle.setAlpha((float) 0.5);
                    }
                }
                if (ScreenSize == 4 && role.contentEquals("mcu")) {
                    mName.setTextAppearance(context, R.style.contactBroadcastName);
                }
                if (role.contentEquals("unknown")) {
                    mName.setAlpha(0.3f);
                    mName.setText("Unknown");
                    mName.setTextAppearance(context, R.style.contactListName_unknow);
                }
                if (titleName != null) {
                    mNameTitle.setText(titleName);
                } else {
                    mNameTitle.setVisibility(View.GONE);
                }

            }

        }//end of CallLogView constructor

        private TextView mName;
        private TextView mNameTitle;
        private ImageView mContactsPhoto;
        private LinearLayout Contacts;
    }//END of CallLogView

    @Override
    protected void onListItemClick(ListView l, final View v, int position, final long id) {
        final QTContact2 contact_info = temp_list.get(position);
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "QT number  : " + contact_info.getPhoneNumber());
        if (contact_info.getRole().contentEquals("virtual")) {
            Log.d(DEBUGTAG, "make video call");
            makeCall(contact_info.getPhoneNumber(), true);
        } else {
            Log.d(DEBUGTAG, "make audio call");
            makeCall(contact_info.getPhoneNumber(), true);//String callerID,boolean enableVideo
        }
        //   finish();
    }//END of onListItemClick


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        overridePendingTransition(0, 0);
        mContext = this;
        Log.d(DEBUGTAG, "onCreate");
        qtalksetting = QtalkUtility.getSettings(this, engine);
        if (qtalksetting.provisionType.contentEquals("?type=hh")) {
            setContentView(com.quanta.qtalk.R.layout.layout_contactlist2_patient);
        } else {
            setContentView(com.quanta.qtalk.R.layout.layout_contactlist2_doctor);
        }
        //hideSysUI();
        CallHistory = (Button) findViewById(R.id.bottom_history);
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

        /*******************************Get dpi*********************************/
        QtalkUtility.initRatios(this);
        dpi = QtalkUtility.densityDpi;
        screenwidth = QtalkUtility.screenWidth;
        screenheight = QtalkUtility.screenHeight;

        xlarge_contacts_gap = screenwidth / 13;

        /*******************************Get Screen Size*********************************/
        ScreenSize = QtalkSettings.ScreenSize;
        Log.d(DEBUGTAG, "screensize = " + ScreenSize + " screenwidth = " + screenwidth + " screenheight = " + screenheight + " dpi = " + dpi);
        if (ScreenSize < 4) {
            photoSize = screenwidth / 6;
        } else {
            photoSize = screenwidth / 8;
        }
        Flag.Reprovision.setState(false);
        Flag.Phonebook.setState(false);

        //mHandler.postDelayed(AvatarUpdateCheck, 10000);
        mHandler.postDelayed(AvatarUpdateCheck, 5000);
    }//end oncreate

    private void hideSysUI() {
        Log.d(DEBUGTAG, "SYSUI hide");
        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        decorView.setSystemUiVisibility(uiOptions);
    }

    public void updateCallAvatar() {
        if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "updateCallAvatar");
        layout_contactlist2 = (RelativeLayout) findViewById(com.quanta.qtalk.R.id.layout_contactlist2);
        first = (LinearLayout) findViewById(com.quanta.qtalk.R.id.first_contact);
        setPhonebook();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return super.onTouchEvent(event);
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(DEBUGTAG, "onDestroy");
        unregisterReceiver(receiver);

        releaseAll();
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(DEBUGTAG, "onResume");
        QtalkSettings.nurse_station_tap_selected = 1;
        Hack.startNursingApp(this);
        updateCallAvatar();
        if (temp_list != null) {
            //ContactListAdapter mContactListAdapter = new ContactListAdapter(this);
            //setListAdapter(mContactListAdapter);
            elv = (ExpandableListView) findViewById(R.id.mExpandableListView);
            LinearLayout bly = (LinearLayout) findViewById(R.id.mBroadcastList);
            setExpandableListView(elv, bly);

        }
        //PackageChecker.CheckInstallX264(this);
        ViroActivityManager.setActivity(this);

    }

    private static Calendar mNewCallCalendar = null;
    private static final Object mMakecallLock = new Object();

    private void setExpandableListView(ExpandableListView elv, LinearLayout bly) {

        //---------------------------------------------------for mcu always top
        if (bly != null && bly.getChildCount() == 0) {
            ContactsView mcucv = null;
            mcucv = new ContactsView(this.getApplicationContext(),
                    "unknown",
                    null,
                    getString(R.string.all_station_broadcast),
                    "",
                    "mcu",
                    "unknown",
                    "1");

            OnClickListener broadcastmcu = new OnClickListener() {

                @Override
                public void onClick(View v) {
                    Bundle bundle = new Bundle();
                    Intent intent = new Intent();
                    intent.setClass(ContactsActivity.this, BroadcastActivity.class);
                    //intent.putExtras(bundle);
                    startActivity(intent);
                }

            };
            mcucv.setOnClickListener(broadcastmcu);
            //	    			bly.setPadding(150, 20, 150, 20);
//	    			mcucv.setPadding(10, 0, 10, 0);
            for (int i = 0; i < 3; i++) {
                if (i == 0) {
                    LayoutParams cv_para = new LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                    mcucv.setGravity(Gravity.CENTER);
                    mcucv.setPadding(10, 0, 10, 0);
                    cv_para.weight = screenwidth / 4;
                    bly.setGravity(Gravity.CENTER);
                    bly.setPadding(xlarge_contacts_gap, 20, xlarge_contacts_gap, 20);
                    bly.addView(mcucv, cv_para);
                } else {
                    LinearLayout empty = new LinearLayout(this);
                    LinearLayout empty_bottom = new LinearLayout(this);
                    empty_bottom.setBackgroundResource(R.drawable.selector_btn_list_contact_xlarge);
                    empty.addView(empty_bottom);
                    LayoutParams empty_para = new LayoutParams(
                            LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);

                    empty.setPadding(10, 0, 10, 0);
                    empty.setVisibility(View.INVISIBLE);

                    empty_para.weight = screenwidth / 4;
                    bly.addView(empty, empty_para);
                }
            }

        }
        //----------------ll_large-----------------------------------for mcu always top

        boolean main_nurse = true;
        boolean main_station = true;

        List<Map<String, String>> groups = new ArrayList<Map<String, String>>();
        Map<String, String> group1 = new HashMap<String, String>();
        group1.put("group", getString(R.string.doctor));
        Map<String, String> group2 = new HashMap<String, String>();
        group2.put("group", getString(R.string.nurse));
        Map<String, String> group3 = new HashMap<String, String>();
        group3.put("group", getString(R.string.patient));
        Map<String, String> group4 = new HashMap<String, String>();
        group4.put("group", getString(R.string.cleaner));
        Map<String, String> group5 = new HashMap<String, String>();
        group5.put("group", "其他");
        groups.add(group1);
        groups.add(group2);
        groups.add(group3);
        groups.add(group4);
        //groups.add(group5);

        List<Map<String, ContactsView>> child_doctor = new ArrayList<Map<String, ContactsView>>();
        List<Map<String, ContactsView>> child_nurse = new ArrayList<Map<String, ContactsView>>();
        List<Map<String, ContactsView>> child_patient = new ArrayList<Map<String, ContactsView>>();
        List<Map<String, ContactsView>> child_staff = new ArrayList<Map<String, ContactsView>>();
        List<Map<String, ContactsView>> child_other = new ArrayList<Map<String, ContactsView>>();

        if (qtalksetting.provisionType.contentEquals("?type=qtfa")
                || qtalksetting.provisionType.contentEquals("?type=ns")) {
            Map<String, ContactsView> child = new HashMap<String, ContactsView>();
            ContactsView mcu = null;
            //put first broadcast
            cv = new ContactsView(this.getApplicationContext(),
                    "unknown",
                    null,
                    getString(R.string.all_station_broadcast),
                    "",
                    "mcu",
                    "unknown",
                    "1");

            OnClickListener broadcast = new OnClickListener() {

                @Override
                public void onClick(View v) {
                    Bundle bundle = new Bundle();
                    Intent intent = new Intent();
                    intent.setClass(ContactsActivity.this, BroadcastActivity.class);
                    //intent.putExtras(bundle);
                    startActivity(intent);
                }

            };
/*	    		cv.setOnClickListener(broadcast);
	    		child.put("child", cv);
	    		child_mcu.add(child);*/
        }
        for (int i = 0; i < temp_list.size(); i++) {
            Map<String, ContactsView> child1Data1 = new HashMap<String, ContactsView>();
            String Role = temp_list.get(i).getRole();
            ContactsView cv = null;
            if (temp_list.get(i).getName().trim().equals("")) {
                continue;
            }
            cv = new ContactsView(this.getApplicationContext(),
                    temp_list.get(i).getImage(),
                    temp_list.get(i).getTitle(),
                    temp_list.get(i).getName(),
                    temp_list.get(i).getAlias(),
                    temp_list.get(i).getRole(),
                    temp_list.get(i).getPhoneNumber(),
                    temp_list.get(i).getOnline());

            final String remoteID = temp_list.get(i).getPhoneNumber();
            final String remoteRole = temp_list.get(i).getRole();
            final boolean online = temp_list.get(i).getOnline().contentEquals("1");
            OnClickListener makecallclick = new OnClickListener() {
                @Override
                public void onClick(View cv) {
                    if (!online)
                        return;
                    SimpleDateFormat sdFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS");
                    synchronized (mMakecallLock) {
                        if (mNewCallCalendar == null) {
                            mNewCallCalendar = Calendar.getInstance();
                            mNewCallCalendar.add(Calendar.SECOND, 5);
                            Log.d(DEBUGTAG, "(mNewCallCalendar = " + sdFormat.format(mNewCallCalendar.getTime()));
                        } else {
                            Calendar now = Calendar.getInstance();
                            Log.d(DEBUGTAG, "now=" + sdFormat.format(now.getTime()) + " lastest call=" + sdFormat.format(mNewCallCalendar.getTime()));
                            if (mNewCallCalendar.compareTo(now) < 0) {
                                mNewCallCalendar.clear();
                                now.add(Calendar.SECOND, 5);
                                mNewCallCalendar = now;
                            } else {
                                Log.d(DEBUGTAG, "the second call is not over 5 seconds. now=" + sdFormat.format(now.getTime()) + " lastest call=" + sdFormat.format(mNewCallCalendar.getTime()));
                                return;
                            }
                        }
                    }
                    Log.d(DEBUGTAG, "2 remoteID: " + remoteID);
                    if (remoteID != null) {
                        if (remoteRole.contentEquals("virtual") || remoteRole.contentEquals("doctor")) {
                            makeCall(remoteID, true);//String callerID,boolean enableVideo, boolean enableCamera
                        } else {
                            if (qtalksetting.provisionType.contentEquals("?type=hh")
                                    && remoteRole.contentEquals("station")) {
                                makeStationCall();
                            } else {
                                makeCall(remoteID, true);//String callerID,boolean enableVideo, boolean enableCamera
                            }
                        }
                    }

                }
            };
            cv.setOnClickListener(makecallclick);

            if (Role.contentEquals("nurse") || Role.contentEquals("station")) {
                if (qtalksetting.provisionType.contentEquals("?type=hh")) {
                    if (Role.contentEquals("nurse")) {
                        if (main_nurse) {
                            main_nurse = false;
                            child1Data1.put("child", cv);
                            child_nurse.add(child1Data1);
                        }
                    } else if (Role.contentEquals("station")) {
                        if (main_station) {
                            main_station = false;
                            child1Data1.put("child", cv);
                            child_nurse.add(child1Data1);
                        }
                    } else {
                        child1Data1.put("child", cv);
                        child_nurse.add(child1Data1);
                    }

                } else {
                    child1Data1.put("child", cv);
                    child_nurse.add(child1Data1);
                }
            } else if (Role.contentEquals("doctor")) {
                child1Data1.put("child", cv);
                child_doctor.add(child1Data1);
            } else if (Role.contentEquals("patient")) {
                child1Data1.put("child", cv);
                child_patient.add(child1Data1);

            } else if (Role.contentEquals("staff")) {
                child1Data1.put("child", cv);
                child_staff.add(child1Data1);
            } else {
                child1Data1.put("child", cv);
                child_other.add(child1Data1);
            }
        }

        List<List<Map<String, ContactsView>>> childs = new ArrayList<List<Map<String, ContactsView>>>();
        if (child_doctor.isEmpty()) { // if there is no doctor
            groups.remove(0);
        } else {
            childs.add(child_doctor);
        }
        childs.add(child_nurse);
        childs.add(child_patient);
        childs.add(child_staff);
        //childs.add(child_other);
        ContactExpandListAdapter viewAdapter = new ContactExpandListAdapter(this, groups,
                childs);

        int width = QtalkUtility.widthPixels;

        elv.setIndicatorBoundsRelative(width - QtalkUtility.dpToPx(70), width - QtalkUtility.dpToPx(5));
        //Log.d(DEBUGTAG,"width:"+width+"GetPixelFromDips(50):"+GetPixelFromDips(50)+"GetPixelFromDips(10):"+GetPixelFromDips(10));


        elv.setAdapter(viewAdapter);
        if (qtalksetting.provisionType.contentEquals("?type=hh")) {
            elv.setOnGroupClickListener(new OnGroupClickListener() { // let patient cant close the expand group
                @Override
                public boolean onGroupClick(ExpandableListView parent, View v,
                                            int groupPosition, long id) {
                    if (groupPosition == 2) {
                        return false;
                    } else {
                        return true;
                    }
                }
            });
        }
        SharedPreferences settings = getSharedPreferences("EXPAND", 0);

        int groupCount = elv.getCount();

        for (int i = 0; i < groupCount; i++) { // let all group default is expand
            boolean isExpanded = settings.getBoolean(groups.get(i).get("group"), true);
            if (groups.get(i).get("group").contentEquals(getString(R.string.broadcast))) {
                isExpanded = true;
            }
            if (isExpanded) {
                elv.expandGroup(i);
            }
        }

    }

    @SuppressLint("NewApi")
    private void releaseAll() {
        if (mContactPhoto != null) {
            //mContactPhoto.getDrawable().setCallback(null);
            mContactPhoto = null;
        }
        mContactListAdapter = null;
        cv = null;
        if (Temp1 != null) {
            Temp1.setCallback(null);
            Temp1 = null;
        }
        engine = null;
        temp_list = null;
        pool.shutdown();
        mContext = null;
        mHandler.removeCallbacks(AvatarUpdateCheck);
        mHandler.removeCallbacks(CheckNotificationCB);
        History = null;

    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            Log.d(DEBUGTAG, "test KeyEvent.KEYCODE_BACK " + keyCode);
            finish();
            overridePendingTransition(0, 0);
        } else if (keyCode == KeyEvent.KEYCODE_HOME) {
            Log.d(DEBUGTAG, "test KeyEvent.KEYCODE_HOME " + keyCode);
            finish();
            overridePendingTransition(0, 0);
        }
        return super.onKeyDown(keyCode, event);
    }

    public void makeStationCall() {
        String callseq = null;
        for (int i = 0; i < temp_list.size(); i++) {
            if (temp_list.get(i).getRole().contentEquals("station")) {
                if (callseq == null) {
                    callseq = temp_list.get(i).getUid();
                } else {
                    callseq = callseq + "::" + temp_list.get(i).getUid();
                }
            }
        }
        if (callseq != null) {
            Bundle bundle = new Bundle();
            bundle.putInt("KEY_COMMAND", 3);
            bundle.putString("KEY_NURSE_UID", callseq);
            bundle.putInt("KEY_TIMEOUT", 10);
            Intent intent = new Intent(mContext, CommandService.class);
            intent.putExtras(bundle);
            mContext.startService(intent);
        }
    }

}
