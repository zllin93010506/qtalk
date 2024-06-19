package com.quanta.qtalk;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;

import com.quanta.qtalk.call.CallEngineFactory;
import com.quanta.qtalk.call.CallState;
import com.quanta.qtalk.call.ICall;
import com.quanta.qtalk.call.ICallEngine;
import com.quanta.qtalk.call.ICallEngineListener;
import com.quanta.qtalk.call.ICallListener;
import com.quanta.qtalk.call.MDP;
import com.quanta.qtalk.media.MediaEngine;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.BusyCallActivity;
import com.quanta.qtalk.ui.LogonStateReceiver;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.QTService;
import com.quanta.qtalk.ui.QThProvisionUtility;
import com.quanta.qtalk.ui.RingingUtil;
import com.quanta.qtalk.util.DeviceIDUtility;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PortManager;
import com.quanta.qtalk.util.QtalkUtility;

import java.io.File;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;

public class QtalkEngine implements ICallEngineListener {
    public static final boolean ENABLE_LOG = false;
    private static final String DEBUGTAG = "QSE_QtalkEngine";
    private final Hashtable<String, ICall> mCallTable = new Hashtable<String, ICall>();
    private IQtalkEngineListener mListener = null;
    private ICallEngine mCallEngine = null;
    private boolean mUserDefaultDailer = false;
    private boolean mLoginSuccess = false;
    private boolean mIsLogout = true;
    private String mDomain = null;
    private String mLoginName = null;
    private ConnectivityManager mConManager = null;
    private static QtalkEngine mInstance = null;
    private static final Handler mHandler = new Handler(Looper.getMainLooper());
    private static boolean mInSession = false;

    private QtalkLogManager mQtalkLogManager = null;
    public static final String LOG_FILE_NAME = "viro.log";
    public static final String UPLOAD_FOLDER_NAME = "viro_log";
    private boolean mPreSipLogin = false;
    private Context mContext = null;

    private QtalkEngine() {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "QtalkEngine:");
    }

    public static final synchronized QtalkEngine getInstance() {
        if (mInstance == null) {
            mInstance = new QtalkEngine();
        }
        return mInstance;
    }

    private boolean isWifiConnected() {
        if (mConManager != null) {
            NetworkInfo ni = mConManager.getActiveNetworkInfo();
            if ((ni == null) || !ni.isConnected())
                return false;

            return ((ni.getType() == ConnectivityManager.TYPE_WIFI));
        }
        return false;
    }

    private boolean isNetworkConnected() {
        if (mConManager != null) {
            NetworkInfo ni = mConManager.getActiveNetworkInfo();
            if ((ni == null) || !ni.isConnected())
                return false;

            return true;
        }
        return false;
    }

    public void setListener(IQtalkEngineListener listener) {
        mListener = listener;
    }

    public synchronized void logout(String msg) {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>logout:" + mCallEngine);
        mIsLogout = true;
        if (mCallEngine != null) {
            mCallEngine.logout();
            if (mCallTable != null) {
                mCallTable.clear();
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "clear mCallTable");
            }
        }
        if (mListener != null) {
            mListener.onLogonStateChange(false, msg, 0);
        }
        mContext = null;
        mCallEngine = null;
        mLoginSuccess = false;
        if (ENABLE_LOG)
            mQtalkLogManager.stopUploadThread();
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<==logout");

        // TODO remove notification
    }

    public synchronized void logout() {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>logout:" + mCallEngine);
        mIsLogout = true;
        if (mCallEngine != null) {
            mCallEngine.logout();
            if (mCallTable != null) {
                mCallTable.clear();
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "clear mCallTable");
            }
        }
        if (mListener != null) {
            mListener.onLogonStateChange(false, "", 0);
        }
        mContext = null;
        mCallEngine = null;
        mLoginSuccess = false;
        if (ENABLE_LOG)
            mQtalkLogManager.stopUploadThread();
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<==logout");

        // TODO remove notification
    }

    public synchronized void resetLogonState() {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>resetLogonState:" + mLoginSuccess);

        if (mListener != null) {
            mListener.onLogonStateChange(false, "Connection Failed", 0);
        }
        mLoginSuccess = false;
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<==resetLogonState");

        // TODO remove notification
    }

    public synchronized void login(Context context) throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>login:" + mCallEngine);

        try {
            Log.d(DEBUGTAG, "caller=" + Thread.currentThread().getStackTrace()[2].getMethodName());
        } catch (Exception e) {

        }

        if (this.isLogon()) {
            Log.d(DEBUGTAG, "QTService now is logon.");
        } else {
            Log.d(DEBUGTAG, "QTService now isnot logon.");
        }

        if (ENABLE_LOG) {
            if (mQtalkLogManager == null && context != null) {
                String deviceID = DeviceIDUtility.getSerialNumber(context);
                mQtalkLogManager = new QtalkLogManager(deviceID,
                    Hack.getStoragePath() + LOG_FILE_NAME,
                    Hack.getStoragePath() + File.separatorChar + UPLOAD_FOLDER_NAME);
                Log.setWriter(mQtalkLogManager);// Setup the log writer
            }
            if (mQtalkLogManager != null)
                mQtalkLogManager.startUploadThread();
        }

        mConManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);

        if (!isNetworkConnected()) {
            Log.e(DEBUGTAG, "No network connection");
            // return;
        }
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(context);
        String authentication_id = preference.getString(QtalkPreference.SETTING_AUTHENTICATION_ID, null);
        String user_id = preference.getString(QtalkPreference.SETTING_SIP_ID, null);
        String password = preference.getString(QtalkPreference.SETTING_PASSWORD, null);
        String realm_server = preference.getString(QtalkPreference.SETTING_REALM_SERVER, null);
        String outbound_proxy = preference.getString(QtalkPreference.SETTING_OUTBOUND_PROXY, null);
        String proxy = preference.getString(QtalkPreference.SETTING_PROXY, null);
        int port = preference.getInt(QtalkPreference.SETTING_PORT, 5060);
        int expire_time = preference.getInt(QtalkPreference.SETTING_REGISTRATION_EXPIRE_TIME, 60);
        mUserDefaultDailer = preference.getBoolean(QtalkPreference.SETTING_USE_DEFAULTDIALER, false);

        int min_audio_port = preference.getInt(QtalkPreference.SETTING_MIN_AUDIO_PORT, 21000);
        int max_audio_port = preference.getInt(QtalkPreference.SETTING_MAX_AUDIO_PORT, 21100);
        int min_video_port = preference.getInt(QtalkPreference.SETTING_MIN_VIDEO_PORT, 20000);
        int max_video_port = preference.getInt(QtalkPreference.SETTING_MAX_VIDEO_PORT, 20100);

        String display_name = preference.getString(QtalkPreference.SETTING_DISPLAY_NAME, null);
        String registrar_server = preference.getString(QtalkPreference.SETTING_REGISTRAR_SERVER, null);
        boolean prack_support = preference.getBoolean(QtalkPreference.SETTING_PRACK_SUPPORT, false);

        NetworkInfo info = mConManager.getActiveNetworkInfo();
        int network_type = 1;
        if (info != null) {
            network_type = info.getType();
            switch (info.getType()) {
                default:
                    break;
                case ConnectivityManager.TYPE_MOBILE:
                    network_type = 1;
                case ConnectivityManager.TYPE_WIFI:
                    network_type = 2;
                case ConnectivityManager.TYPE_ETHERNET:
                    network_type = 3;
                    break;
            }
        } else {
            network_type = 3;
            Log.d(DEBUGTAG, "NetworkInfo info null, set default network type :" + network_type);
        }

        PortManager.setAudioPortRange(min_audio_port, max_audio_port);
        PortManager.setVideoPortRange(min_video_port, max_video_port);

        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "authentication_id:" + authentication_id +
                    ",user_id:" + user_id +
                    ",password:" + password +
                    ",realm_server" + realm_server +
                    ",outbound_proxy:" + outbound_proxy +
                    ",proxy:" + proxy +
                    ",port:" + port +
                    ",expire_time:" + expire_time +
                    ",display name:" + display_name +
                    ",registrar_server:" + registrar_server +
                    ",prack_support:" + prack_support +
                    ",network_type:" + network_type);
        try {
            if (authentication_id != null && user_id != null && password != null && realm_server != null
                    && outbound_proxy != null && proxy != null) {
                mLoginSuccess = false;

                if (mCallEngine == null)
                    mCallEngine = CallEngineFactory.createCallEngine(PortManager.getSIPPort());

                mCallEngine.login(outbound_proxy, proxy, realm_server, port, user_id, authentication_id, password,
                        expire_time, this, display_name, registrar_server, prack_support, network_type);
                mLoginName = authentication_id;
                mDomain = proxy;
                mIsLogout = false;
                mContext = context;
            }
            if (ProvisionSetupActivity.debugMode)
                Log.d(DEBUGTAG, "<==login:" + mCallEngine != null ? "true" : "false");
        } catch (Throwable e) {
            Log.e(DEBUGTAG, "login", e);
            mCallEngine = null;
            throw new FailedOperateException(e);
        }
    }

    public synchronized void updateSetting(Context context, ProvisionInfo provisionInfo) throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>updateSetting:context:" + context + ",provisionInfo:" + provisionInfo);
        if (context != null) {
            // update preference
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    // SIP X18
                    .putString(QtalkPreference.SETTING_DISPLAY_NAME, provisionInfo.display_name)
                    .putString(QtalkPreference.SETTING_SIP_ID, provisionInfo.sip_id)
                    .putString(QtalkPreference.SETTING_AUTHENTICATION_ID, provisionInfo.authentication_id)
                    .putString(QtalkPreference.SETTING_PASSWORD, provisionInfo.authentication_password)
                    .putString(QtalkPreference.SETTING_REALM_SERVER, provisionInfo.sip_realm)
                    .putString(QtalkPreference.SETTING_PROXY, provisionInfo.proxy_server_address)
                    .putInt(QtalkPreference.SETTING_PROXY_PORT, provisionInfo.proxy_server_port)
                    .putInt(QtalkPreference.SETTING_PORT, provisionInfo.proxy_server_port)
                    // .putString( QtalkPreference.SETTING_REGISTRAR_SERVER ,
                    // provisionInfo.registrarAddress)
                    // .putInt( QtalkPreference.SETTING_REGISTRAR_PORT ,
                    // provisionInfo.registrarPORT)
                    .putString(QtalkPreference.SETTING_OUTBOUND_PROXY, provisionInfo.outbound_proxy_address)
                    .putInt(QtalkPreference.SETTING_OUTBOUND_PROXY_PORT, provisionInfo.outbound_proxy_port)
                    // .putBoolean(QtalkPreference.SETTING_OUTBOUND_PROXY_ENABLE ,
                    // provisionInfo.outboundProxyEnalble==1)
                    .putString(QtalkPreference.SETTING_1ST_AUDIO_CODEC, provisionInfo.audio_codec_preference_1)
                    // .putString( QtalkPreference.SETTING_2ND_AUDIO_CODEC ,
                    // provisionInfo.audioCodecPreference2)
                    // .putString( QtalkPreference.SETTING_3RD_AUDIO_CODEC ,
                    // provisionInfo.audioCodecPreference3)
                    // .putString( QtalkPreference.SETTING_4TH_AUDIO_CODEC ,
                    // provisionInfo.audioCodecPreference4)
                    .putString(QtalkPreference.SETTING_1ST_VIDEO_CODEC, provisionInfo.video_codec_preference_1)
                    // .putInt( QtalkPreference.SETTING_LOCAL_SIGNALING_PORT ,
                    // provisionInfo.local_signaling_port)
                    // .putInt(QtalkPreference.SETTING_SIP_TOS, provisionInfo.sipTos)
                    // .putInt(QtalkPreference.SETTING_CHALLENGE_SIP_INVITE,
                    // provisionInfo.challengeSIPInvite)
                    // RTP X7
                    .putInt(QtalkPreference.SETTING_MIN_VIDEO_PORT, provisionInfo.rtp_video_port_range_start)
                    .putInt(QtalkPreference.SETTING_MAX_VIDEO_PORT, provisionInfo.rtp_video_port_range_end)
                    .putInt(QtalkPreference.SETTING_MIN_AUDIO_PORT, provisionInfo.rtp_audio_port_range_start)
                    .putInt(QtalkPreference.SETTING_MAX_AUDIO_PORT, provisionInfo.rtp_audio_port_range_end)
                    .putInt(QtalkPreference.SETTING_OUTGOING_MAX_BANDWIDTH, provisionInfo.outgoing_max_bandwidth)
                    .putBoolean(QtalkPreference.SETTING_AUTO_BANDWIDTH, provisionInfo.auto_bandwidth == 1)
                    .putInt(QtalkPreference.SETTING_VIDEO_MTU, provisionInfo.video_mtu)
                    .putInt(QtalkPreference.SETTING_AUDIO_RTP_TOS, provisionInfo.audio_rtp_tos_setting)
                    .putInt(QtalkPreference.SETTING_VIDEO_RTP_TOS, provisionInfo.video_rtp_tos_setting)
                    // LINES X5
                    .putBoolean(QtalkPreference.SETTING_PRACK_SUPPORT, provisionInfo.prack_support == 1)
                    .putBoolean(QtalkPreference.SETTING_INFO_SUPPORT, provisionInfo.info_support == 1)
                    .putInt(QtalkPreference.SETTING_DTMF_METHOD, provisionInfo.dtmf_transmit_method)
                    .putInt(QtalkPreference.SETTING_REGISTRATION_EXPIRE_TIME,
                            provisionInfo.registration_expiration_time)
                    .putInt(QtalkPreference.SETTING_REGISTRATION_RESEND_TIME, provisionInfo.registration_resend_time)
                    .putInt(QtalkPreference.SETTING_CHALLENGE_SIP_INVITE, provisionInfo.challenge_sip_invite)
                    .putInt(QtalkPreference.SETTING_SESSION_TIMER_ENABLE, provisionInfo.session_timer_enable)
                    .putInt(QtalkPreference.SETTING_SESSION_EXPIRE_TIME, provisionInfo.session_expire_time)
                    .putInt(QtalkPreference.SETTING_MINIMUM_SESSION_EXPIRE_TIME,
                            provisionInfo.minimum_session_expire_time)
                    // CALLS X1
                    /*
                     * .putBoolean(QtalkPreference.SETTING_AUTO_ANSWER ,
                     * provisionInfo.autoAnswer==1)
                     * //NETWORK X6
                     * .putString( QtalkPreference.SETTING_SSH_SERVER , provisionInfo.sshServer)
                     * .putString( QtalkPreference.SETTING_SSH_USERNAME , provisionInfo.sshUserName)
                     * .putString( QtalkPreference.SETTING_SSH_PASSWORD , provisionInfo.sshPassWord)
                     * .putString( QtalkPreference.SETTING_FTP_SERVER , provisionInfo.ftpServer)
                     * .putString( QtalkPreference.SETTING_FTP_USER_NAME ,
                     * provisionInfo.ftpUserName)
                     * .putString( QtalkPreference.SETTING_FTP_PASSWORD , provisionInfo.ftpPassWord)
                     */
                    .putString(QtalkPreference.SETTING_PROVISION_LAST_SYNC_TIME, provisionInfo.lastProvisionFileTime)
                    // .putString( QtalkPreference.SETTING_PROVISIONING_PHONEBOOK_ID ,
                    // provisionInfo.phonebook_group_id)
                    .putString(QtalkPreference.SETTING_PHONEBOOK_LAST_SYNC_TIME, provisionInfo.lastPhonebookFileTime)
                    .putString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_PERIODIC_TIMER,
                            provisionInfo.general_period_check_timer)
                    .putString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_SHORT_PERIODIC_TIMER,
                            provisionInfo.short_period_check_timer)
                    .putString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_DOWNLOAD_ERROR_RETRY_TIMER,
                            provisionInfo.config_file_download_error_retry_timer)
                    .putString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_FORCED_PROVISION_PERIODIC_TIMER,
                            provisionInfo.config_file_forced_provision_periodic_timer)
                    .commit();
            PortManager.setAudioPortRange(provisionInfo.rtp_audio_port_range_start,
                    provisionInfo.rtp_audio_port_range_end);
            PortManager.setVideoPortRange(provisionInfo.rtp_video_port_range_start,
                    provisionInfo.rtp_video_port_range_end);
        } else {
            throw new FailedOperateException("Invalid input context:" + context);
        }
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<===updateSetting");
    }

    public synchronized void updateSetting(Context context, QtalkSettings settings) throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "==>updateSetting:context:" + context + ",settings:" + settings);
        if (context != null) {
            // update preference
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    // SIP X18
                    .putString(QtalkPreference.SETTING_DISPLAY_NAME, settings.displayName)
                    .putString(QtalkPreference.SETTING_SIP_ID, settings.userID)
                    .putString(QtalkPreference.SETTING_AUTHENTICATION_ID, settings.authenticationID)
                    .putString(QtalkPreference.SETTING_PASSWORD, settings.password)
                    .putString(QtalkPreference.SETTING_REALM_SERVER, settings.realmServer)
                    .putString(QtalkPreference.SETTING_PROXY, settings.proxy)
                    .putInt(QtalkPreference.SETTING_PROXY_PORT, settings.proxyPort)
                    .putInt(QtalkPreference.SETTING_PORT, settings.port)
                    .putString(QtalkPreference.SETTING_REGISTRAR_SERVER, settings.registrarServer)
                    .putInt(QtalkPreference.SETTING_REGISTRAR_PORT, settings.registrarPort)
                    .putString(QtalkPreference.SETTING_OUTBOUND_PROXY, settings.outBoundProxy)
                    .putInt(QtalkPreference.SETTING_OUTBOUND_PROXY_PORT, settings.outboundProxyPort)
                    .putBoolean(QtalkPreference.SETTING_OUTBOUND_PROXY_ENABLE, settings.outboundProxyEnable)
                    .putString(QtalkPreference.SETTING_1ST_AUDIO_CODEC, settings.firstACodec)
                    .putString(QtalkPreference.SETTING_2ND_AUDIO_CODEC, settings.secondACodec)
                    .putString(QtalkPreference.SETTING_3RD_AUDIO_CODEC, settings.thirdACodec)
                    .putString(QtalkPreference.SETTING_4TH_AUDIO_CODEC, settings.fourthACodec)
                    .putString(QtalkPreference.SETTING_1ST_VIDEO_CODEC, settings.firstVCodec)
                    // .putInt( QtalkPreference.SETTING_LOCAL_SIGNALING_PORT ,
                    // settings.localSignalingPort)
                    // .putInt(QtalkPreference.SETTING_SIP_TOS, settings.sipTos)
                    // .putInt(QtalkPreference.SETTING_CHALLENGE_SIP_INVITE,
                    // settings.challengeSIPInvite)
                    // RTP X7
                    .putInt(QtalkPreference.SETTING_MIN_VIDEO_PORT, settings.minVideoPort)
                    .putInt(QtalkPreference.SETTING_MAX_VIDEO_PORT, settings.maxVideoPort)
                    .putInt(QtalkPreference.SETTING_MIN_AUDIO_PORT, settings.minAudioPort)
                    .putInt(QtalkPreference.SETTING_MAX_AUDIO_PORT, settings.maxAudioPort)
                    .putInt(QtalkPreference.SETTING_OUTGOING_MAX_BANDWIDTH, settings.outMaxBandWidth)
                    .putBoolean(QtalkPreference.SETTING_AUTO_BANDWIDTH, settings.autoBandWidth)
                    .putInt(QtalkPreference.SETTING_VIDEO_MTU, settings.videoMtu)
                    .putInt(QtalkPreference.SETTING_AUDIO_RTP_TOS, settings.audioRTPTos)
                    .putInt(QtalkPreference.SETTING_VIDEO_RTP_TOS, settings.videoRTPTos)
                    // LINES X5
                    .putBoolean(QtalkPreference.SETTING_PRACK_SUPPORT, settings.prackSupport)
                    .putBoolean(QtalkPreference.SETTING_INFO_SUPPORT, settings.infoSupport)
                    .putInt(QtalkPreference.SETTING_DTMF_METHOD, settings.dtmfMethod)
                    .putInt(QtalkPreference.SETTING_REGISTRATION_EXPIRE_TIME, settings.expireTime)
                    .putInt(QtalkPreference.SETTING_REGISTRATION_RESEND_TIME, settings.resendTime)
                    // CALLS X1
                    .putBoolean(QtalkPreference.SETTING_AUTO_ANSWER, settings.autoAnswer)
                    // NETWORK X6
                    /*
                     * .putString( QtalkPreference.SETTING_SSH_SERVER , settings.sshServer)
                     * .putString( QtalkPreference.SETTING_SSH_USERNAME , settings.sshUserName)
                     * .putString( QtalkPreference.SETTING_SSH_PASSWORD , settings.sshPassWord)
                     * .putString( QtalkPreference.SETTING_FTP_SERVER , settings.ftpServer)
                     * .putString( QtalkPreference.SETTING_FTP_USER_NAME , settings.ftpUserName)
                     * .putString( QtalkPreference.SETTING_FTP_PASSWORD , settings.ftpPassWord)
                     */
                    // OTHERS
                    .putBoolean(QtalkPreference.SETTING_USE_DEFAULTDIALER, settings.useDefaultDialer)
                    .putString(QtalkPreference.SETTING_PROVISION_USER_ID, settings.provisionID)
                    .putString(QtalkPreference.SETTING_PROVISION_PASSWORD, settings.provisionPWD)
                    .putString(QtalkPreference.SETTING_PROVISION_SERVER, settings.provisionServer)
                    .putString(QtalkPreference.SETTING_PROVISION_TYPE, settings.provisionType)
                    .putBoolean(QtalkPreference.SETTING_PROVISION_CAMERA_SWITCH, settings.provisionCameraSwitch)
                    /*
                     * .putString( QtalkPreference.SETTING_PHONEBOOK_SERVER ,
                     * settings.phonebookServer)
                     * .putString( QtalkPreference.SETTING_FIRMWARE_SERVER ,
                     * settings.firmwareServer)
                     */
                    .putInt(QtalkPreference.SETTING_SIGNIN_MODE, settings.signinMode)
                    .putString(QtalkPreference.SETTING_VERSION_NUMBER, settings.versionNumber)
                    .putString(QtalkPreference.SETTING_PROVISION_LAST_SYNC_TIME, settings.lastProvisionFileTime)
                    .putString(QtalkPreference.SETTING_PHONEBOOK_LAST_SYNC_TIME, settings.lastPhonebookFileTime)
                    .putString(QtalkPreference.SETTING_PROVISIONING_SESSION_ID, settings.session)
                    .putString(QtalkPreference.SETTING_PROVISIONING_PHONEBOOK_ID, settings.phonebook_group_id)
                    .commit();
            PortManager.setAudioPortRange(settings.minAudioPort, settings.maxAudioPort);
            PortManager.setVideoPortRange(settings.minVideoPort, settings.maxVideoPort);

            // relogin
            // logout();
            // login(context);
        } else {
            throw new FailedOperateException("Invalid input context:" + context);
        }
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<===updateSetting");
    }

    public QtalkSettings getSettings(Context context) throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "getSettings:" + context);
        QtalkSettings result = null;
        try {
            SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(context);
            if (preference != null) {
                result = new QtalkSettings();
                // SIP X 18
                result.displayName = preference.getString(QtalkPreference.SETTING_DISPLAY_NAME, "");
                result.userID = preference.getString(QtalkPreference.SETTING_SIP_ID, "");
                result.authenticationID = preference.getString(QtalkPreference.SETTING_AUTHENTICATION_ID, "");
                result.password = preference.getString(QtalkPreference.SETTING_PASSWORD, "");
                result.realmServer = preference.getString(QtalkPreference.SETTING_REALM_SERVER, "");
                result.outBoundProxy = preference.getString(QtalkPreference.SETTING_OUTBOUND_PROXY, "");
                result.outboundProxyPort = preference.getInt(QtalkPreference.SETTING_OUTBOUND_PROXY_PORT, 5060);
                result.outboundProxyEnable = preference.getBoolean(QtalkPreference.SETTING_OUTBOUND_PROXY_ENABLE, true);
                result.proxy = preference.getString(QtalkPreference.SETTING_PROXY, "");
                result.proxyPort = preference.getInt(QtalkPreference.SETTING_PROXY_PORT, 5060);
                result.port = preference.getInt(QtalkPreference.SETTING_PORT, 5060);
                result.registrarServer = preference.getString(QtalkPreference.SETTING_REGISTRAR_SERVER, "");
                result.registrarPort = preference.getInt(QtalkPreference.SETTING_REGISTRAR_PORT, 5060);
                // result.sipTos = preference.getInt(QtalkPreference.SETTING_SIP_TOS, 0);
                // result.challengeSIPInvite =
                // preference.getInt(QtalkPreference.SETTING_CHALLENGE_SIP_INVITE, 60);
                result.firstACodec = preference.getString(QtalkPreference.SETTING_1ST_AUDIO_CODEC, "");
                result.secondACodec = preference.getString(QtalkPreference.SETTING_2ND_AUDIO_CODEC, "");
                result.thirdACodec = preference.getString(QtalkPreference.SETTING_3RD_AUDIO_CODEC, "");
                result.fourthACodec = preference.getString(QtalkPreference.SETTING_4TH_AUDIO_CODEC, "");
                result.firstVCodec = preference.getString(QtalkPreference.SETTING_1ST_VIDEO_CODEC, "");
                result.realmServer = preference.getString(QtalkPreference.SETTING_REALM_SERVER, "");
                // result.localSignalingPort = preference.getInt(
                // QtalkPreference.SETTING_LOCAL_SIGNALING_PORT, 5060);
                // RTP X7
                result.minVideoPort = preference.getInt(QtalkPreference.SETTING_MIN_VIDEO_PORT, 20000);
                result.maxVideoPort = preference.getInt(QtalkPreference.SETTING_MAX_VIDEO_PORT, 20100);
                result.minAudioPort = preference.getInt(QtalkPreference.SETTING_MIN_AUDIO_PORT, 21000);
                result.maxAudioPort = preference.getInt(QtalkPreference.SETTING_MAX_AUDIO_PORT, 21100);
                result.outMaxBandWidth = preference.getInt(QtalkPreference.SETTING_OUTGOING_MAX_BANDWIDTH, 512);
                result.autoBandWidth = preference.getBoolean(QtalkPreference.SETTING_AUTO_BANDWIDTH, true);
                result.videoMtu = preference.getInt(QtalkPreference.SETTING_VIDEO_MTU, 980);
                // eason - test un-mark two TOSs
                result.audioRTPTos = preference.getInt(QtalkPreference.SETTING_AUDIO_RTP_TOS, 0);
                result.videoRTPTos = preference.getInt(QtalkPreference.SETTING_VIDEO_RTP_TOS, 0);
                // LINES X5
                result.prackSupport = preference.getBoolean(QtalkPreference.SETTING_PRACK_SUPPORT, false);
                result.infoSupport = preference.getBoolean(QtalkPreference.SETTING_INFO_SUPPORT, true);
                result.dtmfMethod = preference.getInt(QtalkPreference.SETTING_DTMF_METHOD, 0);
                result.expireTime = preference.getInt(QtalkPreference.SETTING_REGISTRATION_EXPIRE_TIME, 60);
                result.resendTime = preference.getInt(QtalkPreference.SETTING_REGISTRATION_RESEND_TIME, 60);
                // CALLS X1
                result.autoAnswer = preference.getBoolean(QtalkPreference.SETTING_AUTO_ANSWER, false);
                // NETWORK
                /*
                 * result.sshServer = preference.getString( QtalkPreference.SETTING_SSH_SERVER,
                 * "");
                 * result.sshUserName = preference.getString(
                 * QtalkPreference.SETTING_SSH_USERNAME, "");
                 * result.sshPassWord = preference.getString(
                 * QtalkPreference.SETTING_SSH_PASSWORD, "");
                 * result.ftpServer = preference.getString( QtalkPreference.SETTING_FTP_SERVER,
                 * "");
                 * result.ftpUserName = preference.getString(
                 * QtalkPreference.SETTING_FTP_USER_NAME, "");
                 * result.ftpPassWord = preference.getString(
                 * QtalkPreference.SETTING_FTP_PASSWORD, "");
                 * 
                 * if(!result.sshServer.trim().equalsIgnoreCase("") &&
                 * result.sshServer.length()>0)
                 * QtalkLogManager.enableSftp(result.sshServer, result.sshUserName,
                 * result.sshPassWord);
                 * else
                 */
                QtalkLogManager.disableSftp();
                // OTHERS
                result.useDefaultDialer = preference.getBoolean(QtalkPreference.SETTING_USE_DEFAULTDIALER, false);
                result.provisionID = preference.getString(QtalkPreference.SETTING_PROVISION_USER_ID, "");
                result.provisionPWD = preference.getString(QtalkPreference.SETTING_PROVISION_PASSWORD, "");
                result.provisionServer = preference.getString(QtalkPreference.SETTING_PROVISION_SERVER, "");
                result.provisionType = preference.getString(QtalkPreference.SETTING_PROVISION_TYPE, "");
                result.provisionCameraSwitch = preference.getBoolean(QtalkPreference.SETTING_PROVISION_CAMERA_SWITCH,
                        false);
                result.provisionCameraSwitchReason = 6;
                /*
                 * result.phonebookServer = preference.getString(
                 * QtalkPreference.SETTING_PHONEBOOK_SERVER, "");
                 * result.firmwareServer = preference.getString(
                 * QtalkPreference.SETTING_FIRMWARE_SERVER, "");
                 */
                // result.isProvisionSignedin = preference.getBoolean(
                // QtalkPreference.QTH_IS_PROVISION_SIGNEDIN, false);
                result.signinMode = preference.getInt(QtalkPreference.SETTING_SIGNIN_MODE, 0);
                result.versionNumber = preference.getString(QtalkPreference.SETTING_VERSION_NUMBER, "");
                // result.isSR2Authenticated =
                // preference.getBoolean(QtalkPreference.SR2_IS_AUTHENTICATED, false);
                result.lastProvisionFileTime = preference.getString(QtalkPreference.SETTING_PROVISION_LAST_SYNC_TIME,
                        "");
                result.lastPhonebookFileTime = preference.getString(QtalkPreference.SETTING_PHONEBOOK_LAST_SYNC_TIME,
                        "");
                result.config_file_periodic_timer = preference
                        .getString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_PERIODIC_TIMER, "");
                result.general_period_check_timer = preference
                        .getString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_GENERAL_TIMER, "");
                result.short_period_check_timer = preference
                        .getString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_SHORT_PERIODIC_TIMER, "");
                result.config_file_download_error_retry_timer = preference
                        .getString(QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_DOWNLOAD_ERROR_RETRY_TIMER, "");
                result.config_file_forced_provision_periodic_timer = preference.getString(
                        QtalkPreference.SETTING_PROVISIONING_CONFIG_FILE_FORCED_PROVISION_PERIODIC_TIMER, "");
                result.session = preference.getString(QtalkPreference.SETTING_PROVISIONING_SESSION_ID, "");
                // result.phonebook_group_id = preference.getString(
                // QtalkPreference.SETTING_PROVISIONING_PHONEBOOK_ID, "");
            }
        } catch (Throwable e) {
            Log.e(DEBUGTAG, "getSettings", e);
            throw new FailedOperateException(e);
        }
        return result;
    }

    public void keepAlive(Context context) {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "keepAlive:" + mIsLogout);
        if (mIsLogout)
            return;
        if (mCallEngine != null) {
            try {
                mCallEngine.keepAlive();
            } catch (Throwable e) {
                // mCallEngine = null;
                Log.e(DEBUGTAG, "keepAlive", e);
            }
        }
        if (mIsLogout == false && mCallEngine == null) {
            try {
                Log.d(DEBUGTAG, "engine.login context==null?" + (context == null));
                login(context);
            } catch (Throwable e) {
                Log.e(DEBUGTAG, "keepAlive", e);
            }
        }
    }

    public synchronized void waitCallReady(final ICall call) {
        int retry = 5;
        while ((call.getCallState() < 0) && (retry-- > 0)) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                Log.e(DEBUGTAG, "", e);
            }
        }
    }

    // makeCall return callID iif success, else return zero or negative error code
    public synchronized void makeCall(String callID, String remoteID, boolean isVideoCall)
            throws FailedOperateException {
        QtalkSettings settings = null;
        boolean enableCameraMute = false;
        ICall call = null;

        settings = QtalkUtility.getSettings(mContext, this);
        if (settings == null)
            Log.d(DEBUGTAG, "makeCall get settings=null");
        else
            Log.d(DEBUGTAG, "makeCall:" + remoteID +
                    ", isVideoCall:" + isVideoCall +
                    ", CameraEnable:" + settings.provisionCameraSwitch +
                    ", mCallEngine:" + mCallEngine +
                    ", mCallTable.size:" + mCallTable.size());

        if (callID != null) {
            call = mCallTable.get(callID);
        }
        if (mCallEngine != null) {
            try {
                if (!(remoteID.startsWith(QtalkSettings.qsptNumPrefix)
                        || remoteID.startsWith(QtalkSettings.MCUPrefix))) {
                    isVideoCall = true;
                }
                if (settings.provisionCameraSwitch)
                    enableCameraMute = false;
                else
                    enableCameraMute = true;

                mCallEngine.makeCall(call,
                        remoteID,
                        mDomain,
                        ((call != null && call.getLocalAudioPort() != 0) ? call.getLocalAudioPort()
                                : MediaEngine.getNextAudioPort()), // MediaEngine.getNextAudioPort()
                        isVideoCall ? (call != null ? MediaEngine.getNextVideoPort() : MediaEngine.getVideoPort()) : 0,
                        false, enableCameraMute);
            } catch (Throwable e) {
                Log.e(DEBUGTAG, "makeCall", e);
                throw new FailedOperateException(e);
            }
        } else {
            QTService.outgoingcallfail();
            throw new FailedOperateException("mCallEngine is null, Call ID=" + remoteID);
        }
    }

    // makeCall return callID iif success, else return zero or negative error code
    public synchronized void makeCall(String callID, String remoteID, boolean isVideoCall, boolean enableCameraMute)
            throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "makeCall:" + remoteID + " isVideoCall:" + isVideoCall + " mCallEngine:" + mCallEngine
                    + ", mCallTable.size:" + mCallTable.size());
        // String result = null;
        ICall call = null;
        if (callID != null) {
            call = mCallTable.get(callID);
        }
        if (mCallEngine != null) {
            try {
                // change V port
                if (!(remoteID.startsWith(QtalkSettings.qsptNumPrefix)
                        || remoteID.startsWith(QtalkSettings.MCUPrefix))) {
                    isVideoCall = true;
                }
                mCallEngine.makeCall(call,
                        remoteID,
                        mDomain,
                        ((call != null && call.getLocalAudioPort() != 0) ? call.getLocalAudioPort()
                                : MediaEngine.getNextAudioPort()), // MediaEngine.getNextAudioPort()
                        isVideoCall ? (call != null ? MediaEngine.getNextVideoPort() : MediaEngine.getVideoPort()) : 0,
                        false, enableCameraMute);
                // invite without changing V/A ports
                // mCallEngine.makeCall(call,
                // remoteID,
                // mDomain,
                // ((call!=null&&call.getLocalAudioPort()!=0)?call.getLocalAudioPort():MediaEngine.getAudioPort()),
                // isVideoCall?((call!=null&&call.getLocalVideoPort()!=0)?call.getLocalVideoPort():MediaEngine.getVideoPort()):0,
                // false);
            } catch (Throwable e) {
                Log.e(DEBUGTAG, "makeCall", e);
                throw new FailedOperateException(e);
            }
        } else {
            QTService.outgoingcallfail();
            throw new FailedOperateException("mCallEngine is null, Call ID=" + remoteID);
        }
        // return result;
    }

    public synchronized void makeSeqCall(String callID, String remoteID, boolean isVideoCall, String DisplayName)
            throws FailedOperateException {
        Log.d(DEBUGTAG, "makeSeqCall:" + remoteID + ", isVideoCall:" + isVideoCall + ", mCallEngine:" + mCallEngine
                + ", mCallTable.size:" + mCallTable.size() + ", DisplayName:" + DisplayName);

        // String result = null;
        boolean enableCameraMute = false;
        QtalkSettings settings = null;
        if (mContext != null) {
            settings = QtalkUtility.getSettings(mContext, this);
        } else {
            QTService.outgoingcallfail();
            throw new FailedOperateException("mContext is null");
        }
        ICall call = null;
        if (callID != null) {
            call = mCallTable.get(callID);
        }
        if (mCallEngine != null) {
            try {
                // change V port
                if (!(remoteID.startsWith(QtalkSettings.qsptNumPrefix)
                        || remoteID.startsWith(QtalkSettings.MCUPrefix))) {
                    isVideoCall = true;
                }

                Log.d(DEBUGTAG, "settings.provisionCameraSwitch:" + settings.provisionCameraSwitch + " reason:"
                        + settings.provisionCameraSwitchReason);
                if (settings.provisionCameraSwitch)
                    enableCameraMute = false;
                else
                    enableCameraMute = true;
                mCallEngine.makeSeqCall(call,
                        DisplayName,
                        remoteID,
                        mDomain,
                        ((call != null && call.getLocalAudioPort() != 0) ? call.getLocalAudioPort()
                                : MediaEngine.getNextAudioPort()), // MediaEngine.getNextAudioPort()
                        isVideoCall ? (call != null ? MediaEngine.getNextVideoPort() : MediaEngine.getVideoPort()) : 0,
                        false, enableCameraMute);
                // invite without changing V/A ports
                // mCallEngine.makeCall(call,
                // remoteID,
                // mDomain,
                // ((call!=null&&call.getLocalAudioPort()!=0)?call.getLocalAudioPort():MediaEngine.getAudioPort()),
                // isVideoCall?((call!=null&&call.getLocalVideoPort()!=0)?call.getLocalVideoPort():MediaEngine.getVideoPort()):0,
                // false);
            } catch (Throwable e) {
                Log.e(DEBUGTAG, "makeSeqCall", e);
                throw new FailedOperateException(e);
            }
        } else {
            QTService.outgoingcallfail();
            throw new FailedOperateException("mCallEngine is null, Call ID=" + remoteID);
        }
        // return result;
    }

    public synchronized void acceptCall(String callID, String remoteID, boolean isVideoCall, boolean enableCameraMute)
            throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG,
                    "acceptCall:" + callID + " isVideoCall:" + isVideoCall + " enableCameraMute:" + enableCameraMute);
        ICall call = mCallTable.get(callID);
        if (call != null) {
            if (mCallEngine != null) {
                try {
                    mCallEngine.acceptCall(call,
                            (call != null && call.getLocalAudioPort() != 0) ? call.getLocalAudioPort()
                                    : MediaEngine.getAudioPort(),
                            isVideoCall ? ((call != null && call.getLocalVideoPort() != 0) ? call.getLocalVideoPort()
                                    : MediaEngine.getVideoPort()) : 0,
                            enableCameraMute);
                } catch (Throwable e) {
                    Log.e(DEBUGTAG, "acceptCall", e);
                    throw new FailedOperateException(e);
                }
            } else {
                throw new FailedOperateException("mCallEngine is null, Call ID=" + callID);
            }
        } else {
            throw new FailedOperateException("Call doesn't exist. Call ID=" + callID);
        }
    }

    public synchronized void hangupCall(String callID, String remoteID, boolean isReinvite)
            throws FailedOperateException {
        Log.d(DEBUGTAG, "QtalkEngine.hangupCall(), callID:" + callID + ", remoteID:" + remoteID + ", mCallTable.size:"
                + mCallTable.size());

        // ------------------------------------QF3 LED
        Intent intent = new Intent();
        intent.setAction("android.intent.action.vc.led");
        Bundle bundle = new Bundle();
        bundle.putString("KEY_LED_VALUE", "0");
        intent.putExtras(bundle);
        Log.d(DEBUGTAG, "send led event to launcher:2");
        if (mContext != null)
            mContext.sendBroadcast(intent);
        // ------------------------------------QF3 LED

        if (callID != null) {
            ICall call = mCallTable.get(callID);

            if (mCallEngine != null && call != null) {
                mCallEngine.hangupCall(call, isReinvite);
            }
            if (!isReinvite) {
                mCallTable.remove(callID);
                Flag.InsessionFlag.setState(false);
                Flag.PTT.setState(false);
                Flag.MCU.setState(false);
                if (call != null) { // outgoing call or hang up call when session is established.
                                    // if(!(QtalkSettings.nurseCall))
                    mListener.onLogCall(call.getRemoteID(), call.getRemoteDisplayName(), call.getCallType(),
                            call.getStartTime(), call.isVideoCall());
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG,
                                "hangupCall remoteID:" + call.getRemoteID() + " Call Type:" + call.getCallType());
                }
            }

            /*
             * if (mCallEngine!=null && call!=null)
             * {
             * mCallEngine.hangupCall(call,isReinvite);
             * }
             */

        }
        if (ENABLE_LOG) {
            mQtalkLogManager.uploadLog();
        }
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "<==hangupCall:" + mCallTable.size());
    }

    public synchronized void holdCall(String callID, String remoteID, boolean isHold) throws FailedOperateException {
        if (ProvisionSetupActivity.debugMode)
            Log.d(DEBUGTAG, "holdCall:" + remoteID + " mCallEngine:" + mCallEngine);
        // String result = null;
        ICall call = null;
        if (callID != null)
            call = mCallTable.get(callID);
        if (mCallEngine != null) {
            try {
                // this method is called only in audio session, so set 5th parameter in makecall
                // as "0"
                mCallEngine.makeCall(call,
                        remoteID,
                        mDomain,
                        ((call != null && call.getLocalAudioPort() != 0) ? call.getLocalAudioPort()
                                : MediaEngine.getAudioPort()),
                        (call != null && call.isVideoCall() == false) ? 0
                                : ((call != null && call.getLocalVideoPort() != 0) ? call.getLocalVideoPort()
                                        : MediaEngine.getVideoPort()),
                        isHold,
                        false);
            } catch (Throwable e) {
                Log.e(DEBUGTAG, "holdCall", e);
                throw new FailedOperateException(e);
            }
        } else {
            throw new FailedOperateException("mCallEngine is null, Call ID=" + remoteID);
        }
        // return result;
    }

    public boolean replaceSystemOutgoingCall() {
        return mUserDefaultDailer;
    }

    public boolean isLogon() {
        return mLoginSuccess;
    }

    public String getLogonName() {
        return mLoginName;
    }

    public void eixt() {
        if (mListener != null)
            mListener.onExit();
    }

    public boolean isCallExisted(String callID) {
        return mCallTable.containsKey(callID);
    }

    public boolean isInSession() {
        return mInSession;
    }

    public boolean setCallListener(String callID, ICallListener listener) {
        ICall call = null;
        if (callID != null) {
            call = mCallTable.get(callID);
        }
        if (call != null && call.getCallState() != CallState.HANGUP) {
            call.setListener(listener);
            return true;
        } else {
            if (call == null) {
                return false;
            }
            if (QtalkSettings.nurseCall) { // for PT call transfer
                call.setListener(listener);
                return true;
            } // end
            return false;
        }
    }

    public void demandIDR(String callID) {
        Log.d(DEBUGTAG, "onsend onDemandIDR");
        ICall call = null;
        if (callID != null) {
            call = mCallTable.get(callID);
        }
        if (call != null && mCallEngine != null) {
            mCallEngine.requestIDR(call);
        }
    }

    // ===============implements ICallEngineListener
    // public java.util.Hashtable<int, ICall> mCallTable;
    @Override
    public void onLogonState(final boolean state, final String msg, final int expireTime) {
        mHandler.post(new Runnable() {
            public void run() {
                Log.e(DEBUGTAG, "Identity SIP return 200 ok or not.");
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "==>onLogonState:" + state);
                if (state == false) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.e(DEBUGTAG, "==>onLogonState:" + state + "sip un-connect");
                    // Log.e(DEBUGTAG, "SIP login failed! Going to logout App!");
                    // logout("網路無法連線\r\n"); // [PT] we do not logout sip
                    mPreSipLogin = false;
                    Log.e(DEBUGTAG, "SIP login failed!");
                    Flag.SIPRunnable.setState(false);
                    if (mCallTable != null) {
                        Log.d(DEBUGTAG, "mCallTable.size = " + mCallTable.size());
                        // mCallTable.clear();
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "clear mCallTable");
                    }
                    LogonStateReceiver.notifyLogonState(mContext, state,
                            null, msg);
                } else if (state && !Flag.SIPRunnable.getState()) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.e(DEBUGTAG, "==>onLogonState:" + state + "sip un-connect");
                    Flag.SIPRunnable.setState(true);
                    LogonStateReceiver.notifyLogonState(mContext, state,
                            null, msg);
                }
                if (state == true && mPreSipLogin == false) { // for PT
                    LogonStateReceiver.notifyLogonState(mContext, true, null, "QSPT-VC");
                    mPreSipLogin = true;
                } else if (Hack.isPhone()) {
                    LogonStateReceiver.notifyLogonState(mContext, state,
                            null, msg);
                }

                if (mLoginSuccess != state && Flag.SIPRunnable.getState() == true) {
                    // Log.d("QtalkMain", "Flag.RunnableExist:"+Flag.RunnableExist.getState());
                    mLoginSuccess = state;
                    if (mLoginSuccess) {
                        try { // Refine the audio codec priority according to connection type(Wifi or 3.5G)
                            MDP[] mdps = MediaEngine.getAudioMDP();
                            if (!isWifiConnected() && mdps.length > 1) {
                                MDP tmp = mdps[1];
                                mdps[1] = mdps[0];
                                mdps[0] = tmp;
                            }
                            if (Hack.isMotorolaAtrix()) {
                                Vector<MDP> mdp_vector = new Vector<MDP>();
                                for (int i = 0; i < mdps.length; i++) {
                                    if (mdps[i].ClockRate == 8000) {
                                        mdp_vector.add(mdps[i]);
                                    }
                                }
                                mdps = new MDP[mdp_vector.size()];
                                for (int i = 0; i < mdps.length; i++) {
                                    mdps[i] = mdp_vector.get(i);
                                }
                            }
                            if (mCallEngine != null) {
                                if ((Hack.isK72() || Hack.isQF3()) && QtalkSettings.enable1832) {
                                    mCallEngine.setCapability(mdps, MediaEngine.getVideoMDP(), 1280, 720, 30);
                                } else if (Hack.isQF3a() || Hack.isQF5()) {
                                    mCallEngine.setCapability(mdps, MediaEngine.getVideoMDP(), 1280, 720, 30);
                                } else {
                                    // mCallEngine.setCapability( mdps, MediaEngine.getVideoMDP(),1280,720,30);
                                    mCallEngine.setCapability(mdps, MediaEngine.getVideoMDP(), 640, 480, 15);
                                }
                                /* setProvsion to mCallEngine */
                                if (QThProvisionUtility.provision_info != null)
                                    mCallEngine.setProvision(QThProvisionUtility.provision_info);
                                else if (ProvisionSetupActivity.debugMode)
                                    Log.e(DEBUGTAG, "provision_info is null");
                            } else {
                                if (ProvisionSetupActivity.debugMode)
                                    Log.e(DEBUGTAG, "mCallEngine is null, capability has not set and reset flag");
                                Flag.SIPRunnable.setState(false);
                                mLoginSuccess = false;

                            }
                        } catch (FailedOperateException e) {
                            Log.e(DEBUGTAG, "onLogonState", e);
                            mLoginSuccess = false;
                        }
                    }
                }
                if (mListener != null) {
                    // mListener.onLogonStateChange(state, msg,expireTime); //[PT] FOR PT we do not
                    // call this function
                }
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "<==onLogonState");
            }
        });
    }

    @Override
    public void onCallIncoming(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onCallIncoming callID:" + (call == null ? "null" : call.getCallID()));
        // if (ENABLE_DEBUG) Log.d(DEBUGTAG,"onCallIncoming:"+call);
        mHandler.post(new Runnable() {
            public void run() {
                if (call != null) {
                    // check if any call exists already
                    Log.d(DEBUGTAG, "mCallTable.size()=" + mCallTable.size());
                    if (mCallTable.size() == 0 ||
                            mCallTable.get(call.getCallID()) != null) {
                        if (mListener != null && call != null) {
                            ICall old_call = mCallTable.put(call.getCallID(), call);
                            // Log.d(DEBUGTAG, "remote display name:"+call.getRemoteDisplayName());
                            mListener.onNewIncomingCall(call.getCallID(), call.getRemoteID(), call.isVideoCall());
                        }
                    } else {
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "Auto reject call:" + mCallTable.size() + ", call ID:"
                                    + mCallTable.get(call.getCallID()));
                        // if call exists, reject new incoming call
                        try {
                            mCallEngine.hangupCall(call, false);
                        } catch (FailedOperateException e) {
                            Log.e(DEBUGTAG, "onCallIncoming", e);
                        }
                    }
                }
            }
        });
    }

    @Override
    public void onCallMCUIncoming(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onCallIncoming callID:" + (call == null ? "null" : call.getCallID()));
        // if (ENABLE_DEBUG) Log.d(DEBUGTAG,"onCallIncoming:"+call);
        mHandler.post(new Runnable() {
            public void run() {
                if (call != null) {
                    // check if any call exists already
                    Log.d(DEBUGTAG, "mCallTable.size()=" + mCallTable.size());
                    if (mCallTable.size() == 0 ||
                            mCallTable.get(call.getCallID()) != null) {
                        if (mListener != null && call != null) {
                            ICall old_call = mCallTable.put(call.getCallID(), call);
                            // Log.d(DEBUGTAG, "remote display name:"+call.getRemoteDisplayName());
                            mListener.onNewMCUIncomingCall(call.getCallID(), call.getRemoteID(), call.isVideoCall());
                        }
                    } else {
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "Auto reject call:" + mCallTable.size() + ", call ID:"
                                    + mCallTable.get(call.getCallID()));
                        // if call exists, reject new incoming call
                        try {
                            mCallEngine.hangupCall(call, false);
                        } catch (FailedOperateException e) {
                            Log.e(DEBUGTAG, "onCallIncoming", e);
                        }
                    }
                }
            }
        });
    }

    @Override
    public void onCallPTTIncoming(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onCallPTTIncoming callID:" + (call == null ? "null" : call.getCallID()));
        // if (ENABLE_DEBUG) Log.d(DEBUGTAG,"onCallIncoming:"+call);
        mHandler.post(new Runnable() {
            public void run() {
                if (call != null) {
                    // check if any call exists already
                    Log.d(DEBUGTAG, "mCallTable.size()=" + mCallTable.size());
                    if (mCallTable.size() == 0 ||
                            mCallTable.get(call.getCallID()) != null) {
                        if (mListener != null && call != null) {
                            ICall old_call = mCallTable.put(call.getCallID(), call);
                            // Log.d(DEBUGTAG, "remote display name:"+call.getRemoteDisplayName());
                            mListener.onNewPTTIncomingCall(call.getCallID(), call.getRemoteID(), call.isVideoCall());
                        }
                    } else {
                        if (ProvisionSetupActivity.debugMode)
                            Log.d(DEBUGTAG, "Auto reject call:" + mCallTable.size() + ", call ID:"
                                    + mCallTable.get(call.getCallID()));
                        // if call exists, reject new incoming call
                        mListener.onLogCall(call.getRemoteID(), call.getRemoteDisplayName(), 3, call.getStartTime(),
                                call.isVideoCall());// default to accept
                        try {
                            mCallEngine.hangupCall(call, false);
                        } catch (FailedOperateException e) {
                            Log.e(DEBUGTAG, "onCallIncoming", e);
                        }
                    }
                }
            }
        });
    }

    @Override
    public void onCallOutgoing(final ICall call) {
        Log.l(DEBUGTAG, "onCallOutgoing, callID:" + (call == null ? "null" : call.getCallID()));

        mHandler.post(new Runnable() {
            public void run() {
                if (mListener != null && call != null) {
                    waitCallReady(call);
                    if (call.getCallState() >= 0) {
                        ICall old_call = mCallTable.put(call.getCallID(), call);
                    }
                    mListener.onNewOutGoingCall(call.getCallID(), call.getRemoteID(), call.getCallState(),
                            call.isVideoCall());
                } else {
                    Log.d(DEBUGTAG, "FAILED!, mListener:" + mListener + ", call:" + call);
                }
            }
        });
    }

    @Override
    public void onCallMCUOutgoing(final ICall call) {

    }

    @Override
    public void onCallAnswered(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG,
                    "onCallAnswered callID:" + (call == null ? "null" : call.getCallID()) + ", onCallAnswered: audio:"
                            + (call == null ? 0 : call.getRemoteAudioPort()) + " video:"
                            + (call == null ? "" : call.getRemoteAudioPort()));
        mHandler.post(new Runnable() {
            public void run() {
                if (mListener != null && call != null) {
                    // Check my outgoing call
                    if (mCallTable.get(call.getCallID()) != null) {
                        mCallTable.put(call.getCallID(), call);
                    }
                }
            }
        });
    }

    @Override
    public void onCallHangup(final ICall call, final String msg) {
        if (mContext == null || mIsLogout == true) {
            Log.d(DEBUGTAG, "mContext:" + mContext + " , mIsLogout:" + mIsLogout);
            return;
        }
        // ------------------------------------QF3 LED
        Intent intent = new Intent();
        intent.setAction("android.intent.action.vc.led");
        Bundle bundle = new Bundle();
        bundle.putString("KEY_LED_VALUE", "0");
        intent.putExtras(bundle);
        Log.d(DEBUGTAG, "send led event to launcher:2");
        mContext.sendBroadcast(intent);
        // ------------------------------------QF3 LED
        mHandler.post(new Runnable() {
            public void run() {
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG,
                            "==>onCallHangup:" + call + ", msg:" + msg + ", mCallTable.size:" + mCallTable.size());
                // For BusyCall Popup windows
                if (mContext == null || mIsLogout == true) {
                    Log.d(DEBUGTAG, "mContext:" + mContext + " , mIsLogout:" + mIsLogout);
                    return;
                }
                if (msg != null) {
                    Bundle bundle = new Bundle();
                    if (msg.contentEquals("BYE"))
                        bundle.putString("BusyType", "BYE");
                    else if (msg.startsWith("BUSY") || msg.startsWith("busy"))
                        bundle.putString("BusyType", "Busy");
                    else if (msg.startsWith("Cancel") || msg.startsWith("cancel"))
                        bundle.putString("BusyType", null);
                    else
                        bundle.putString("BusyType", "No");

                    if (bundle.getString("BusyType") != null) {
                        if (call != null)
                            bundle.putString("BusyImage",
                                    Hack.getStoragePath() + call.getRemoteID() + ".jpg.jpg");
                        if (!msg.contentEquals("BYE")) {
                            RingingUtil.playSoundBusyTone(mContext);
                        }
                        bundle.putString("BusyID", call.getRemoteID());
                        Intent BusyIntent = new Intent();
                        BusyIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        BusyIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
                        BusyIntent.putExtras(bundle);
                        BusyIntent.setClass(mContext, BusyCallActivity.class);
                        mContext.startActivity(BusyIntent);
                    }
                }

                if (call != null) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "call id:" + call.getCallID());
                }
                if (ProvisionSetupActivity.debugMode)
                    Log.l(DEBUGTAG, "onCallHangup callID:" + call == null ? "null" : call.getCallID() + ", msg:" + msg);
                if (call != null) {
                    ICall local_call = mCallTable.get(call.getCallID());
                    mCallTable.remove(call.getCallID());
                    Flag.InsessionFlag.setState(false);
                    Flag.PTT.setState(false);
                    Flag.MCU.setState(false);
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "mCallTable" + mCallTable);
                    if (mListener != null) {
                        if (local_call != null) {
                            mListener.onLogCall(call.getRemoteID(), call.getRemoteDisplayName(), call.getCallType(),
                                    call.getStartTime(), call.isVideoCall());
                            if (ProvisionSetupActivity.debugMode)
                                Log.d(DEBUGTAG, "onCallHangup remoteID:" + call.getRemoteID() + " Call Type:"
                                        + call.getCallType());
                        }
                        // mListener.onRemoteHangup(call.getCallID(),msg);
                    }
                    if (ENABLE_LOG)
                        mQtalkLogManager.uploadLog();
                }
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "<==onCallHangup:" + mCallTable.size());
            }
        });
    }

    @Override
    public void onCallStateChange(ICall call, int state, final String msg) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onCallStateChange callID:" + (call == null ? "null" : call.getCallID()) + " state:" + state
                    + " msg:" + msg);
        // if (ENABLE_DEBUG) Log.d(DEBUGTAG,"onCallStateChange:"+state+",call:"+call);
        // TODO Auto-generated method stub

    }

    @Override
    public void onSessionStart(final String callID, final int callType, final String remoteID,
            final String audioDestIP, final int audioLocalPort, final int audioDestPort, final MDP audio,
            final String videoDestIP, final int videoLocalPort, final int videoDestPort, final MDP video,
            final int productType, final long startTime,
            final int width, final int height, final int fps, final int kbps, final boolean enablePLI,
            final boolean enableFIR, final boolean enableTelEvt,
            final boolean enableCameraMute) {
        if (ProvisionSetupActivity.debugMode) {
            Log.l(DEBUGTAG, "onSessionStart callID:" + callID + ", remoteID:" + remoteID + ", callType:" + callType
                    + ", productType:" + productType);
            Log.l(DEBUGTAG, "audioDestIP:" + audioDestIP + ", audioLocalPort:" + audioLocalPort + ", audioDestPort:"
                    + audioDestPort);
            Log.l(DEBUGTAG, "videoDestIP:" + videoDestIP + ", videoLocalPort:" + videoLocalPort + ", videoDestPort:"
                    + videoDestPort);
            Log.l(DEBUGTAG,
                    "productType:" + productType + ", startTime:" + startTime + ", width:" + width + ", height:"
                            + height + ", fps:" + fps + ", kbps:" + kbps + ", enablePLI:" + enablePLI + ", enableFIR:"
                            + enableFIR + ", enableCameraMute:" + enableCameraMute);
            Log.l(DEBUGTAG, "FECCtrl:" + audio.FEC_info.FECCtrl + ", FECSendPType:" + audio.FEC_info.FECSendPayloadType
                    + ", FECRecvPType:" + audio.FEC_info.FECRecvPayloadType);
            Log.l(DEBUGTAG, "SRTPenable:" + audio.SRTP_info.enableSRTP + ", SRTPsendKey:" + audio.SRTP_info.SRTPSendKey
                    + ", SRTPreceiveKey:" + audio.SRTP_info.SRTPReceiveKey);
        }
        // ------------------------------------QF3 LED
        Intent intent = new Intent();
        intent.setAction("android.intent.action.vc.led");
        Bundle bundle = new Bundle();
        bundle.putString("KEY_LED_VALUE", "1");
        intent.putExtras(bundle);
        Log.d(DEBUGTAG, "send led event to launcher:1");
        mContext.sendBroadcast(intent);
        // ------------------------------------QF3 LED
        mHandler.post(new Runnable() {
            public void run() {
                if (audio != null) {
                    if (video != null && videoLocalPort != 0 && videoDestPort != 0) {
                        mListener.onNewVideoSession(callID, callType, remoteID,
                                audioDestIP, audioLocalPort, audioDestPort, audio,
                                videoDestIP, videoLocalPort, videoDestPort, video, productType, startTime,
                                width, height, fps, kbps, enablePLI, enableFIR, enableTelEvt, enableCameraMute);
                    } else {
                        mListener.onNewAudioSession(callID, callType, remoteID,
                                audioDestIP, audioLocalPort, audioDestPort, audio, startTime, enableTelEvt,
                                productType);
                    }
                }
            }
        });
        mInSession = true;
    }

    @Override
    public void onMCUSessionStart(final String callID, final int callType, final String remoteID,
            final String audioDestIP, final int audioLocalPort, final int audioDestPort, final MDP audio,
            final String videoDestIP, final int videoLocalPort, final int videoDestPort, final MDP video,
            final int productType, final long startTime,
            final int width, final int height, final int fps, final int kbps, final boolean enablePLI,
            final boolean enableFIR, final boolean enableTelEvt) {

    }

    @Override
    public void onSessionEnd(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onSessionEnd callID:" + call == null ? "null" : call.getCallID());
        mHandler.post(new Runnable() {
            public void run() {
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "==>onSessionEnd:" + call + ", mCallTable.size:" + mCallTable.size());
                if (call != null) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "call id:" + call.getCallID());
                }
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "mCallTable" + mCallTable);
                // mCallTable.remove(call.getCallID());
                if (mListener != null && call != null)
                    mListener.onSessionEnd(call.getCallID());
                if (ENABLE_LOG && mQtalkLogManager != null)
                    mQtalkLogManager.uploadLog();
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "<==onSessionEnd:" + mCallTable.size());
            }
        });
        mInSession = false;
    }

    @Override
    public void onMCUSessionEnd(final ICall call) {
        if (ProvisionSetupActivity.debugMode)
            Log.l(DEBUGTAG, "onMCUSessionEnd callID:" + call == null ? "null" : call.getCallID());
        mHandler.post(new Runnable() {
            public void run() {
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "==>onMCUSessionEnd:" + call + ", mCallTable.size:" + mCallTable.size());
                if (call != null) {
                    if (ProvisionSetupActivity.debugMode)
                        Log.d(DEBUGTAG, "call id:" + call.getCallID());
                }
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "mCallTable" + mCallTable);
                // mCallTable.remove(call.getCallID());
                if (mListener != null && call != null)
                    mListener.onMCUSessionEnd(call.getCallID());
                if (ENABLE_LOG)
                    mQtalkLogManager.uploadLog();
                mCallTable.remove(call.getCallID());
                if (ProvisionSetupActivity.debugMode)
                    Log.d(DEBUGTAG, "<==onMCUSessionEnd:" + mCallTable.size());
            }
        });
        mInSession = false;
    }

    public void sr2_switch_network_interface(int interface_type, String local_ip_addr) {
        if (mCallEngine != null)
            mCallEngine.sr2_switch_network_interface(interface_type, local_ip_addr);
    }

    public void qth_switch_call_to_px(final String mCallID, String pX_SIP_ID) {
        ICall call = null;
        if (mCallID != null)
            call = mCallTable.get(mCallID);

        // Log.l(DEBUGTAG,"qth_switch_call_to_px
        // callID:"+call==null?"null":call.getCallID());
        if (mCallEngine != null)
            mCallEngine.qth_switch_call_to_px(call, pX_SIP_ID);

    }

    @Override
    public void onCallTransferToPX(ICall call, int success) {
        Log.d(DEBUGTAG, "onCallTransferToPX:" + success);
        /*
         * if( success == 1)
         * {
         * //if (call.getCallID()!=null )
         * //call = mCallTable.get(call.getCallID());
         * Log.d(TAG,"onCallTransferToPX: remove CallID:"+call.getCallID());
         * 
         * mCallTable.remove(call.getCallID());
         * }
         */
    }

    public static boolean isServiceExisted(Context context, String className) {
        ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningServiceInfo> serviceList = activityManager.getRunningServices(Integer.MAX_VALUE);

        int size = serviceList.size();
        for (int i = 0; i < size; ++i) {
            ComponentName serviceName = serviceList.get(i).service;
            Log.d("Service test", "Service: " + serviceName.getClassName());
            if (serviceName.getClassName().equals(className)) {
                return true;
            }
        }
        return false;
    }
}
