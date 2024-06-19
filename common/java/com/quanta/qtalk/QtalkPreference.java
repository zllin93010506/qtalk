package com.quanta.qtalk;

public class QtalkPreference 
{
    // SharedPreferences file name
    public final static String FILE_NAME                     = "com.quanta.qtalk.QtalkPreference";
    
    // For Account Setting & Manual Config
    //SIP
    public final static String SETTING_DISPLAY_NAME          = "DISPLAY_NAME";
    public final static String SETTING_SIP_ID                = "SIP_ID";
    public final static String SETTING_AUTHENTICATION_ID     = "AUTHENTICATION_ID";
    public final static String SETTING_PASSWORD              = "PASSWORD";
    public final static String SETTING_REALM_SERVER          = "REALM_SERVER";
    public final static String SETTING_PROXY                 = "PROXY";
    public final static String SETTING_PROXY_PORT            = "PROXY_PORT";
    public final static String SETTING_OUTBOUND_PROXY        = "OUTBOUND_PROXY";
    public final static String SETTING_OUTBOUND_PROXY_PORT   = "OUTBOUND_PROXY_PORT";
    public final static String SETTING_OUTBOUND_PROXY_ENABLE = "OUTBOUND_PROXY_ENABLE";
    public final static String SETTING_PORT                  = "PORT";
    public final static String SETTING_1ST_AUDIO_CODEC       = "1ST_AUDIO_CODEC";
    public final static String SETTING_2ND_AUDIO_CODEC       = "2ND_AUDIO_CODEC";
    public final static String SETTING_3RD_AUDIO_CODEC       = "3RD_AUDIO_CODEC";
    public final static String SETTING_4TH_AUDIO_CODEC       = "4TH_AUDIO_CODEC";
    public final static String SETTING_1ST_VIDEO_CODEC       = "1ST_VIDEO_CODEC";
    public final static String SETTING_REGISTRAR_SERVER      = "REGISTRAR_SERVER";
    public final static String SETTING_REGISTRAR_PORT        = "REGISTRAR_PORT";
 //   public final static String SETTING_LOCAL_SIGNALING_PORT  = "LOCAL_SIGNALING_PORT";
    public final static String TAG_DNS_RETRY_COYNTS_TO_FAILOVER    		        = "DNS_RETRY_COYNTS_TO_FAILOVER";
    public final static String TAG_PROVISIONING_GENERAL_PERIOD_CHECK_TIMER        = "PROVISIONING_GENERAL_PERIOD_CHECK_TIMER";
    public final static String TAG_PROVISIONING_SHORT_PERIOS_CHECK_TIMER          = "PROVISIONING_SHORT_PERIOS_CHECK_TIMER";
    
    //RTP
    public final static String SETTING_MIN_VIDEO_PORT        = "MIN_VIDEO_PORT";
    public final static String SETTING_MAX_VIDEO_PORT        = "MAX_VIDEO_PORT";
    public final static String SETTING_MIN_AUDIO_PORT        = "MIN_AUDIO_PORT";
    public final static String SETTING_MAX_AUDIO_PORT        = "MAX_AUDIO_PORT";
    public final static String SETTING_AUDIO_RTP_TOS         = "AUDIO_RTP_TOS";
    public final static String SETTING_VIDEO_RTP_TOS         = "VIDEO_RTP_TOS";
    public final static String SETTING_OUTGOING_MAX_BANDWIDTH = "OUTGOING_MAX_BANDWIDTH";
    public final static String SETTING_INCOMING_MAX_BANDWIDTH = "INCOMING_MAX_BANDWIDTH";
    public final static String SETTING_VIDEO_MTU             = "VIDEO_MTU";
    public final static String SETTING_AUTO_BANDWIDTH        = "AUTO_BANDWIDTH";
    
    //LINE
    public final static String SETTING_REGISTRATION_EXPIRE_TIME = "EXPIRE_TIME";
    public final static String SETTING_REGISTRATION_RESEND_TIME = "RESEND_TIME";
    public final static String SETTING_DTMF_METHOD           = "DTMF_METHOD";
    public final static String SETTING_PRACK_SUPPORT         = "PRACK_SUPPORT";
    public final static String SETTING_INFO_SUPPORT          = "INFO_SUPPORT";
    public final static String SETTING_CHALLENGE_SIP_INVITE      = "CHALLENGE_SIP_INVITE";
    public final static String SETTING_SESSION_TIMER_ENABLE      = "SESSION_TIMER_ENABLE ";
    public final static String SETTING_SESSION_EXPIRE_TIME       = "SESSION_EXPIRE_TIME";
    public final static String SETTING_MINIMUM_SESSION_EXPIRE_TIME    = "MINIMUM_SESSION_EXPIRE_TIME";
    
    //CALLS
    public final static String SETTING_AUTO_ANSWER           = "AUTO_ANSWER";
    
    //NETWORK
    public final static String SETTING_SSH_SERVER            = "SSH_SERVER";
    public final static String SETTING_SSH_USERNAME          = "SSH_USERNAME";
    public final static String SETTING_SSH_PASSWORD          = "SSH_PASSWORD";
//    public final static String SETTING_SSH_LOG_RETRY         = "SSH_LOG_RETRY";
    public final static String SETTING_FTP_SERVER            = "FTP_SERVER";
    public final static String SETTING_FTP_USER_NAME         = "FTP_USER_NAME";
    public final static String SETTING_FTP_PASSWORD          = "FTP_PASSWORD";
    
    //SIGNIN MODE
    public final static String SETTING_SIGNIN_MODE           = "SIGNIN_MODE";
    public final static String SETTING_VERSION_NUMBER        = "VERSION_NUMBER";
    public final static String SETTING_PROVISION_SESSION_ID  = "PROVISION_SESSION_ID";
    // FIRMWARE UPDATE
    public final static String SETTING_NEW_FIRMWARE_VERSION  = "NEW_FIRMWARE_VERSION";
    public final static String SETTING_URL_FIRMWARE_DOWNLOAD = "URL_FIRMWARE_DOWNLOAD";
    
    // For Provision Account Setting    
    public final static String SETTING_USE_DEFAULTDIALER     = "USE_DEFAULTDIALER";
    public final static String SETTING_PROVISION_USER_ID     = "PROVISION_USER_ID";
    public final static String SETTING_PROVISION_PASSWORD    = "PROVISION_PASSWORD";
    public final static String SETTING_PROVISION_SERVER      = "PROVISION_SERVER";
    public final static String SETTING_PROVISION_TYPE        = "PROVISION_TYPE";
    public final static String SETTING_PROVISION_CAMERA_SWITCH  = "PROVISION_CAMERA_SWITCH";
/*    public final static String SETTING_PHONEBOOK_SERVER      = "PHONEBOOK_SERVER";
    public final static String SETTING_FIRMWARE_SERVER      = "FIRMWARE_SERVER";*/
    public final static String SETTING_PROVISION_LAST_SYNC_TIME = "PROVISION_LAST_SYNC_TIME";
    public final static String SETTING_PHONEBOOK_LAST_SYNC_TIME	= "PHONEBOOK_LAST_SYNC_TIME";
    
    public static final String SETTING_PROVISIONING_CONFIG_FILE_PERIODIC_TIMER        = "config_file_periodic_timer"; // added by CDRC
    public static final String SETTING_PROVISIONING_CONFIG_FILE_GENERAL_TIMER        = "general_period_check_timer";
    public static final String SETTING_PROVISIONING_CONFIG_FILE_SHORT_PERIODIC_TIMER = "short_period_check_timer";
    public static final String SETTING_PROVISIONING_CONFIG_FILE_DOWNLOAD_ERROR_RETRY_TIMER     = "config_file_download_error_retry_timer"; // added by CDRC
    public static final String SETTING_PROVISIONING_CONFIG_FILE_FORCED_PROVISION_PERIODIC_TIMER        = "config_file_forced_provision_periodic_timer"; // added by CDRC
    public static final String SETTING_PROVISIONING_SESSION_ID 		= "PROVISION_SESSION_ID"; 
    public static final String SETTING_RM_SERVICE_URI	= "RM_SERVICE_URI"; 
    
    //for rm
    public static final String SETTING_PROVISIONING_PHONEBOOK_ID	= "PHONEBOOK_ID";
    
    // for SR2
/*    public final static String SR2_AUTHENTICATED_PX_MAC		 = "AUTHENTICATED_PX_MAC";
    public final static String SR2_IS_AUTHENTICATED			 = "IS_AUTHENTICATED";
    public final static String SR2_AUTHENTICATED_PX_PIN		 = "ooo";
    public final static String QTH_IS_PROVISION_SIGNEDIN	 = "QTH_IS_PROVISION_SIGNEDIN";*/
}
