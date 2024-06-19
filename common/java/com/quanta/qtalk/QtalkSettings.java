package com.quanta.qtalk;

import com.quanta.qtalk.util.Hack;

public class QtalkSettings
{
    //SIP
	public String  authenticationID     = null;
	public String  userID                = null;
	public String  password             = null;
	public String  realmServer          = null;
	public String  proxy                = null;
	public int     proxyPort            = 5060;
	public int     port                 = 5060;
	public String  displayName          = null;
	public String  firstACodec          = null;
	public String  secondACodec         = null;
	public String  thirdACodec          = null;
	public String  fourthACodec         = null;
	public String  firstVCodec          = null;
	public String  registrarServer      = null;
	public int     registrarPort        = 0;
    public String  outBoundProxy        = null;
	public int     outboundProxyPort    = 0;
	public boolean outboundProxyEnable  = true;
	public int     localSignalingPort   = 0;
	
	//RTP
	public int     minVideoPort         = 20000;
	public int     maxVideoPort         = 20100;
	public int     minAudioPort         = 21000;
	public int     maxAudioPort         = 21100;
    public int     audioRTPTos          = 0;
    public int     videoRTPTos          = 0;
    public int     outMaxBandWidth      = 0;
    public int     videoMtu             = 0;
    public boolean autoBandWidth        = true;

	//Lines
	public int     expireTime           = 60;
	public int     resendTime           = 0;
	public int     dtmfMethod           = 2;
//	public int     challengeSIPInvite   = 0;
    public boolean prackSupport         = false;
    public boolean infoSupport          = true;
    
    //Calls
    public boolean autoAnswer           = false;
    
 /*   //Network
    public String  sshServer            = null;
    public String  sshUserName          = null;
    public String  sshPassWord          = null;
    public String  ftpServer            = null;
    public String  ftpUserName          = null;
    public String  ftpPassWord          = null;*/
    
    //OTHERS
	public int     signinMode           = 0;
	public String  versionNumber        = null;
	public boolean useDefaultDialer     = false;
	public String  provisionID          = null;
	public String  provisionPWD         = null;
	public String  provisionServer      = null;
	public String  provisionType 		 = null;
	public boolean  provisionCameraSwitch = false; // PT
	public static boolean control_keep_visible = false;
	public int     provisionCameraSwitchReason = 0;
/*	public String  phonebookServer      = null;
	public String  firmwareServer      = null;*/
	public String  session			= null;
	public String  key			= null;
	public String  phonebook_group_id        = null; // added by CDRC
	
	public String lastProvisionFileTime = "00000000000000000"; // time format = YYYYMMDDHHmmSS000, initialize to 0 here
	public String lastPhonebookFileTime = "00000000000000000"; // time format = YYYYMMDDHHmmSS000, initialize to 0 here
	public String provision_keepalive_expiretime	= null;
    public String config_file_periodic_timer             = null;
    public String general_period_check_timer             = null;
    public String short_period_check_timer             = null;
    public String config_file_download_error_retry_timer    = null;
    public String config_file_forced_provision_periodic_timer             = null;
	//Firmware Update
	public static String  newFirmwareVersion   =null;
	public static String  urlFirmwareDownload   =null;
	
	// SR2 
	public String PX_MAC_ADDRESS	= null; 
	public boolean isSR2Authenticated	= false;
	public boolean isProvisionSignedin = false;
	
	// PT
	public static boolean Login_by_launcher = false;
	public static boolean makecall_by_launcher = false;
	public static String uidCall = "";
	public static boolean nurseCall = false;
	public static String nurseCall_uid = "";
	public static int timeout = 25; // second
	public static int ScreenSize = -1;
	public static boolean enable1832 = true;
	public static String qsptNumPrefix = "5912";
	public static String MCUPrefix = "22822281";

	public static String  ptmsServer = "";
	public static String  tts_deviceId = "";
	public static String  type = "";

	public static int nurse_station_tap_selected = 0;
}
