package com.quanta.qtalk.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils.TruncateAt;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.ui.contact.QTDepartment;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.QtalkUtility;

import org.xml.sax.SAXException;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.ParserConfigurationException;

public class BroadcastActivity extends Activity {
    private static final String DEBUGTAG = "QTFa_BroadcastActivity";
    private String provisionID = null;
    private String provisionType = null;
    private String broadcast_uri = null;
    private String room_number = null;
    private static Context mContext = null;
    private List<String> ttslist;
    private ArrayList<QTDepartment> deplist = new ArrayList<QTDepartment>();
    private String ttsmessage = null;
    RelativeLayout tts_check_xlarge;
    private static final int MCU_SUCCESS = 9;
    private static final int MCU_FAIL = 10;
    private static final int MCU_BUSY = 11;
    private static final int MCU_IN_PROGRESS = 12;
    private int ScreenSize = 0, screenwidth = 0, screenheight = 0;


    AlertDialog connect_popout = null,
            text_popout = null,
            list_popout = null,
            ptt_popout = null,
            tts_popout = null,
            choose_popout = null;
    Button ptt_btn = null;
    ScrollView tts_scl = null;
    private ArrayList<String> depID = new ArrayList<String>();

    private BroadcastReceiver receiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                //finish();
                Log.d(DEBUGTAG, "reveive ACTION_CLOSE_SYSTEM_DIALOGS");
            } else if (intent.getAction().equals(LauncherReceiver.ACTION_VC_TTS_CB)) {
                mHandler.removeCallbacks(ttsCBtimeoutThread);
                Bundle bundle = intent.getExtras();
                String msg = bundle.getString("KEY_STATE");
                if (msg != null) {
                    Log.d(DEBUGTAG, "receive tts cb:" + msg);
                    if (msg.contentEquals("SUCCESS")) {
                        mHandler.postDelayed(ttsSuccessThread, 1000);
                        mHandler.postDelayed(finishThread, 2000);
                    } else {
                        mHandler.postDelayed(ttsFailThread, 1000);
                        mHandler.postDelayed(finishThread, 2000);
                    }
                }
            }
        }
    };

    private final Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MCU_SUCCESS:
                    String MCU = msg.getData().getString("room_number");
                    Log.d(DEBUGTAG, "MCU number" + MCU);
                    if (MCU != null) {
                        try {
                            QTReceiver.engine(mContext, false).makeCall(null, MCU,
                                    false, false);
                        } catch (FailedOperateException e) {
                            Log.e(DEBUGTAG, "makeCall", e);
                        }
                    }
                    mHandler.postDelayed(finishThread, 3000);
                    break;
                case MCU_FAIL:
                    Log.d(DEBUGTAG, "broadcast mcu fail!!!!!!");
                    openTextDialog(getString(R.string.fail_send_ptt));
                    mHandler.postDelayed(finishThread, 2000);
                    connect_popout.dismiss();
                case MCU_BUSY:
                    Log.d(DEBUGTAG, "broadcast mcu fail!!!!!!");
                    openTextDialog(getString(R.string.no_available_broadcast_agent));
                    mHandler.postDelayed(finishThread, 2000);
                    connect_popout.dismiss();
                case MCU_IN_PROGRESS:
                    Log.d(DEBUGTAG, "broadcast mcu fail!!!!!!");
                    openTextDialog(getString(R.string.department_during_broadcasting));
                    mHandler.postDelayed(finishThread, 2000);
                    connect_popout.dismiss();
                    break;
            }
        }
    };

    private RadioGroup.OnCheckedChangeListener mTtsChangeRadio = new RadioGroup.OnCheckedChangeListener() {

        @Override
        public void onCheckedChanged(RadioGroup group, int checkedId) {

            tts_check_xlarge.setVisibility(View.VISIBLE);
            ttsmessage = ttslist.get(checkedId).toString();
            ptt_btn.setVisibility(View.GONE);
            //RelativeLayout broadcast_bg = (RelativeLayout)findViewById(R.id.broadcast_bg);
            //broadcast_bg.setBackgroundResource(R.drawable.img_popout_broadcast_tablet_2);

        }

    };
    private RadioGroup.OnCheckedChangeListener mDepChangeRadio = new RadioGroup.OnCheckedChangeListener() {

        @Override
        public void onCheckedChanged(RadioGroup group, int checkedId) {

        }

    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(DEBUGTAG, "OnCreate");
        mContext = this;
        QtalkUtility.initRatios(this);
        ttslist = new ArrayList<String>();
        setContentView(R.layout.layout_broadcast);
        Intent intent = new Intent();
        intent.setAction(LauncherReceiver.ACTION_GET_TTS);
        sendBroadcast(intent);
        String path = Hack.getStoragePath() + "ttsList";
        try {
            BufferedReader br = new BufferedReader(new FileReader(path));
            while (br.ready()) {
                String item = br.readLine();


                Log.d(DEBUGTAG, item);
                ttslist.add(item);
            }
            br.close();
        } catch (Exception e) {
            Log.e(DEBUGTAG, "", e);
        }

        try {
            deplist = ProvisionUtility.parseProvisionDepartment(Hack.getStoragePath() + "provision.xml");
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

        ScreenSize = QtalkSettings.ScreenSize;
        screenwidth = QtalkUtility.screenWidth;
        screenheight = QtalkUtility.screenHeight;
    }

    private void sendttsmessage() {
        if (ttsmessage != null) {
            if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "ttsmessage=" + ttsmessage);

            tts_check_xlarge.setVisibility(View.GONE);

            IntentFilter intentfilter = new IntentFilter();
            intentfilter.addAction(LauncherReceiver.ACTION_VC_TTS_CB);
            registerReceiver(receiver, intentfilter);
            openTextDialog(getString(R.string.tts_sending));

            Intent intent = new Intent();
            intent.setAction(LauncherReceiver.ACTION_VC_TTS);
            Bundle bundle = new Bundle();
            bundle.putString("KEY_TTS_MSG", ttsmessage);
            bundle.putStringArrayList("KEY_NS_ID", depID);
            intent.putExtras(bundle);
            sendBroadcast(intent);
            mHandler.postDelayed(ttsCBtimeoutThread, 5000);

        }
    }

    protected void finishActivity() {
        finish();
    }

    @Override
    protected void onStart() {
        Log.d(DEBUGTAG, "OnStart");
        super.onStart();
        if (deplist != null && deplist.size() > 1) {
            openChooseDepDialog();
        } else {
            if (deplist != null && deplist.size() == 1)
                depID.add(deplist.get(0).getDepID());
            openBroadcastListDialog();
        }

    }

    @Override
    protected void onResume() {
        Log.d(DEBUGTAG, "OnResume");
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        releaseAll();
        super.onDestroy();
    }

    private void releaseAll() {
        if (connect_popout != null) {
            connect_popout.dismiss();
            connect_popout = null;
        }
        if (text_popout != null) {
            text_popout.dismiss();
            text_popout = null;
        }
        if (list_popout != null) {
            list_popout.dismiss();
            list_popout = null;
        }
        if (ptt_popout != null) {
            ptt_popout.dismiss();
            ptt_popout = null;
        }
        if (tts_popout != null) {
            tts_popout.dismiss();
            tts_popout = null;
        }
        if (choose_popout != null) {
            choose_popout.dismiss();
            choose_popout = null;
        }

    }

    Runnable sendMCU = new Runnable() {
        @Override
        public void run() {
            sendMCURequest();
        }
    };

    private void sendMCURequest() {
        Log.d(DEBUGTAG, "sendMCURequest");
        try {
            final QtalkSettings qtalk_settings = QTReceiver.engine(this, false)
                    .getSettings(this);
            provisionID = qtalk_settings.provisionID;
            provisionType = qtalk_settings.provisionType;
        } catch (FailedOperateException e1) {
            // TODO Auto-generated catch block
            Log.e(DEBUGTAG, "", e1);
        }
        //[temp] only one dep will be ppt
        String dep = null;
        if (depID != null && depID.size() > 0) {
            dep = depID.get(0);
            Log.d(DEBUGTAG, "dep:" + depID.get(0));
        }

        if (QThProvisionUtility.provision_info != null) {
            broadcast_uri = QThProvisionUtility.provision_info.phonebook_uri
                    + provisionType + "&service=broadcastmcu&uid="
                    + provisionID + "&action=get_room_number&dep_id=" + dep;
            try {
                QThProvisionUtility qtnMessenger = new QThProvisionUtility(this, mHandler,
                        null);
                qtnMessenger.httpsConnect(broadcast_uri, room_number);
            } catch (Exception e) {
                Log.e(DEBUGTAG, "BroadcastMCU", e);
                sendPTTwithoutNetwork();
                Log.e(DEBUGTAG, "", e);
            }
        } else {
            String provision_file = Hack.getStoragePath()
                    + "provision.xml";
            try {
                QThProvisionUtility.provision_info = ProvisionUtility
                        .parseProvisionConfig(provision_file);
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

    }

    private void sendPTTwithoutNetwork() {
        mHandler.postDelayed(finishThread, 1000);
        Intent intent = new Intent(mContext, OutgoingCallFailActivity.class);
        Bundle bundle = new Bundle();
        bundle.putInt("KEY_ERRORCODE", 1);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtras(bundle);
        mContext.startActivity(intent);
    }

    final Runnable finishThread = new Runnable() {
        public void run() {
            finish();
        }
    };
    final Runnable ttsCBtimeoutThread = new Runnable() {
        public void run() {
            Log.d(DEBUGTAG, "ttsCB timeout");
            unregisterReceiver(receiver);
            openTextDialog(getString(R.string.fail_send_tts));
            mHandler.postDelayed(finishThread, 2000);
        }
    };
    final Runnable ttsSuccessThread = new Runnable() {
        public void run() {
            openTextDialog(getString(R.string.success_send_tts));
            unregisterReceiver(receiver);
        }
    };
    final Runnable ttsFailThread = new Runnable() {
        public void run() {
            openTextDialog(getString(R.string.fail_send_tts));
            unregisterReceiver(receiver);
            ;
        }
    };

    void openPTTcheckDialog() {
        if (ptt_popout == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    BroadcastActivity.this);
            ptt_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                    .setCancelable(true).create();
        }
        ptt_popout.show();

        Window win = ptt_popout.getWindow();
        win.setContentView(R.layout.broadcast_ptt_dialogue);
        Button btn_ptt_cancel = (Button) win.findViewById(R.id.btn_close);
        btn_ptt_cancel.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                finish();
            }
        });
        Button btn_ptt_comfirm = (Button) win.findViewById(R.id.btn_ptt_confirm);
        btn_ptt_comfirm.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                Log.d(DEBUGTAG, "onClick sendMCURequest");
                new Thread(sendMCU).start();
                openPTTconnectDialog();
                ptt_popout.dismiss();
            }
        });

        OnCancelListener cancellistener = new OnCancelListener() {

            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        };

        ptt_popout.setOnCancelListener(cancellistener);
    }

    void openPTTconnectDialog() {
        if (connect_popout == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    BroadcastActivity.this);
            connect_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                    .setCancelable(false).create();
        }
        connect_popout.show();

        Window win = connect_popout.getWindow();
        win.setContentView(R.layout.broadcast_ptt_connect_dialogue);

    }

    void openTextDialog(String msg) {
        if (text_popout == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    BroadcastActivity.this);
            text_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                    .setCancelable(false).create();
        }
        text_popout.show();

        Window win = text_popout.getWindow();
        win.setContentView(R.layout.broadcast_text_dialogue);
        TextView tv = (TextView) win.findViewById(R.id.text_content);
        tv.setText(msg);

    }

    void openChooseDepDialog() {
        if (choose_popout == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    BroadcastActivity.this);
            choose_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                    .setCancelable(true).create();
        }
        choose_popout.show();
        Window win = choose_popout.getWindow();
        win.setContentView(R.layout.broadcast_choose_dep_dialogue);
        Button btn_close = (Button) win.findViewById(R.id.btn_close);
        final RadioGroup depRadioGroup = (RadioGroup) win.findViewById(R.id.depRadioGroup);
        btn_close.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                finish();
            }
        });
        if (deplist != null) {
            if (depRadioGroup.getChildCount() == 0) {
                for (int i = 0; i < deplist.size(); i++) {

                    RadioButton chkBshow = new RadioButton(this);

                    chkBshow.setText(deplist.get(i).getDepName());
                    chkBshow.setTextAppearance(mContext, R.style.ttsList);
                    chkBshow.setSingleLine(true);
                    chkBshow.setButtonDrawable(R.drawable.trans1x1);
                    chkBshow.setBackgroundResource(R.drawable.rdbcheck_background);
                    chkBshow.setId(i);
                    chkBshow.setPadding(40, 0, 0, 0);
                    depRadioGroup.addView(chkBshow);
                }

                depRadioGroup.setOnCheckedChangeListener(mDepChangeRadio);
            }
        }

        OnCancelListener cancellistener = new OnCancelListener() {

            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        };

        choose_popout.setOnCancelListener(cancellistener);


    }

    void openBroadcastListDialog() {
        if (list_popout == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    BroadcastActivity.this);
            list_popout = builder.setTitle(R.string.app_name).setMessage("waiting")
                    .setCancelable(true).create();
        }
        list_popout.show();

        Window win = list_popout.getWindow();
        win.setContentView(R.layout.broadcast_list_dialogue);

        RelativeLayout broadcast_popout_content = (RelativeLayout) win.findViewById(R.id.broadcast_popout_content);
        broadcast_popout_content.getLayoutParams().width = QtalkUtility.ratioWidth(781);
        broadcast_popout_content.getLayoutParams().height = QtalkUtility.ratioHeight(454);
        //broadcast_popout_content.setBackgroundColor(Color.YELLOW);

        ptt_btn = (Button) win.findViewById(R.id.ptt_btn);
        tts_scl = (ScrollView) win.findViewById(R.id.tts_ll);
        //tts_scl.setBackgroundColor(Color.BLUE);

        tts_scl.getLayoutParams().width = QtalkUtility.ratioWidth(781);
        tts_scl.getLayoutParams().height = QtalkUtility.ratioHeight(385);

        //ptt_btn.setBackgroundColor(Color.GREEN);
        int leftPadding = QtalkUtility.ratioWidth(45);
        int topPadding = QtalkUtility.ratioHeight(11);
        //ptt_btn.setPadding(leftPadding, topPadding, 0, 0);
        ptt_btn.setTextSize(QtalkUtility.ratioHFont(28));
        RelativeLayout.LayoutParams ptt_btn_param = (RelativeLayout.LayoutParams) ptt_btn.getLayoutParams();
        ptt_btn_param.width = QtalkUtility.ratioWidth(688);
        ptt_btn_param.height = QtalkUtility.ratioHeight(45);
        ptt_btn_param.setMargins(leftPadding, topPadding, 0, 0);
        ptt_btn.setLayoutParams(ptt_btn_param);

        final RadioGroup ttsRadioGroup = (RadioGroup) win.findViewById(R.id.ttsRadioGroup);
        TextView empty_message = (TextView) win.findViewById(R.id.empty_message);
        Button btn_close = (Button) win.findViewById(R.id.btn_close);
        ptt_btn.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                openPTTcheckDialog();
                list_popout.dismiss();
            }

        });

        if (ttslist != null) {
            if (ttsRadioGroup.getChildCount() == 0) {
                for (int i = 0; i < ttslist.size(); i++) {

                    RadioButton chkBshow = new RadioButton(this);

                    chkBshow.setText(ttslist.get(i).toString());
                    //chkBshow.setMarqueeRepeatLimit(6);
                    chkBshow.setEllipsize(TruncateAt.END);
                    chkBshow.setTextAppearance(mContext, R.style.ttsList);
                    chkBshow.setSingleLine(true);
                    //chkBshow.setSelected(true);
                    chkBshow.setButtonDrawable(R.drawable.trans1x1);
                    chkBshow.setBackgroundResource(R.drawable.rdbcheck_background);
                    chkBshow.setId(i);

                    RadioGroup.LayoutParams params = new RadioGroup.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                    int hMargin = QtalkUtility.ratioWidth(40);
                    params.setMargins(hMargin, 0, hMargin, 0);
                    chkBshow.setLayoutParams(params);
                    int hPadding = QtalkUtility.ratioWidth(16);
                    chkBshow.setPadding(hPadding, 0, hPadding, 0);
                    chkBshow.setCompoundDrawablePadding(hPadding);
                    //chkBshow.setPadding(QtalkUtility.ratioWidth(40), 0, QtalkUtility.ratioWidth(100), 0);
                    //chkBshow.setMaxWidth(300);
                    chkBshow.setSelected(true);
                    chkBshow.setMarqueeRepeatLimit(6);
                    chkBshow.setEllipsize(TruncateAt.MARQUEE);

                    ttsRadioGroup.addView(chkBshow);
                }

                ttsRadioGroup.setOnCheckedChangeListener(mTtsChangeRadio);
            }
            if (ttslist.size() == 0) {
                empty_message.setVisibility(View.VISIBLE);
            } else {
                empty_message.setVisibility(View.GONE);
            }
        }
        btn_close.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                finish();
            }
        });

        tts_check_xlarge = (RelativeLayout) win.findViewById(R.id.rl_tts_xlarge_check);
		//tts_check_xlarge.setBackgroundColor(Color.RED);
        RelativeLayout.LayoutParams tts_check_xlarge_para = (RelativeLayout.LayoutParams) tts_check_xlarge.getLayoutParams();


        tts_check_xlarge_para.topMargin = topPadding;
        tts_check_xlarge_para.height = QtalkUtility.ratioHeight(45);

        Button btn_tts_cancel_xlarge = (Button) win.findViewById(R.id.btn_tts_cancel_xlarge);
        btn_tts_cancel_xlarge.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                finish();
            }
        });
        Button btn_tts_comfirm_xlarge = (Button) win.findViewById(R.id.btn_tts_confirm_xlarge);
        btn_tts_comfirm_xlarge.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                sendttsmessage();

            }
        });

        tts_check_xlarge.setVisibility(View.GONE);


        OnCancelListener cancellistener = new OnCancelListener() {

            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        };

        list_popout.setOnCancelListener(cancellistener);
    }

}