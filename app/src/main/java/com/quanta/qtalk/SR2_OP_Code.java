package com.quanta.qtalk;

public class SR2_OP_Code {
	public final static int PX2PBLIMIT = 100;
	
	public final static int OP_CODE_CLOSE_ACTIVITY = 900;
	public final static int OP_CODE_NEW_PX_DEVICE = 901;
	public final static int OP_CODE_FINISH_SCANNING = 902;
	public final static int OP_CODE_REGISTER = 400;
	public final static int OP_CODE_AUTHENTICATION_REQUIRED = 401;
	public final static int OP_CODE_AUTHENTICATION_OK = 402;
	public final static int OP_CODE_HANDSET_BYE = 404;
	public final static int OP_CODE_XML_FILE_READ_FINISH = 201;
	public final static int OP_CODE_XML_FILE_READ_PB_FINISH = 200;
	public final static int OP_CODE_XML_FILE_READ_CH_FINISH = 203;
	public final static int OP_CODE_XML_FILE_PB_TRANS_FINISH = 210;
	public final static int OP_CODE_XML_FILE_CH_TRANS_FINISH = 213;
	public final static int OP_CODE_XML_FILE_TRANS_FINISH = 202;
	public final static int OP_CODE_MAKE_VOICE_CALL = 100;
	
	public final static int OP_CODE_VOICECALL_HOLD = 101;
	public final static int OP_CODE_VOICECALL_ONHOLD = 102;
	public final static int OP_CODE_VOICECALL_HOLD_RESUME = 103;
	public final static int OP_CODE_VOICECALL_ONHOLD_RESUME = 104;
	public final static int OP_CODE_VOICECALL_MUTE = 105;
	public final static int OP_CODE_VOICECALL_TERMINATION = 106;
	public final static int OP_CODE_VOICECALL_CANCEL = 107;
	public final static int OP_CODE_VOICECALL_REJECT = 108;
	public final static int OP_CODE_VOICECALL_DTMF = 109;
	public final static int OP_CODE_VOICECALL_SWITCH_REQUEST = 110;
	public final static int OP_CODE_VOICECALL_SWITCH_CONFIRM = 111;
	public final static int OP_CODE_VOICECALL_CONFERENCE = 112;
	public final static int OP_CODE_VOICECALL_TRANSFER = 113;
	public final static int OP_CODE_VOICECALL_SESSION = 114;
	public final static int OP_CODE_VOICECALL_MAKECALL_RESULT = 115;
	public final static int OP_CODE_VOICECALL_UNMUTE = 116;
	
	public final static int OP_CODE_MAKE_VIDEO_CALL = 200;
	
	public final static int OP_CODE_VIDEOCALL_HOLD = 201;
	public final static int OP_CODE_VIDEOCALL_ONHOLD = 202;
	public final static int OP_CODE_VIDEOCALL_HOLD_RESUME = 203;
	public final static int OP_CODE_VIDEOCALL_ONHOLD_RESUME = 204;
	public final static int OP_CODE_VIDEOCALL_MUTE = 205;
	public final static int OP_CODE_VIDEOCALL_TERMINATION = 206;
	public final static int OP_CODE_VIDEOCALL_CANCEL = 207;
	public final static int OP_CODE_VIDEOCALL_REJECT = 208;
	public final static int OP_CODE_VIDEOCALL_DTMF = 209;
	public final static int OP_CODE_VIDEOCALL_SWITCH_REQUEST = 210;
	public final static int OP_CODE_VIDEOCALL_SWITCH_CONFIRM = 211;
	public final static int OP_CODE_VIDEOCALL_CONFERENCE = 212;
	public final static int OP_CODE_VIDEOCALL_TRANSFER = 213;
	public final static int OP_CODE_VIDEOCALL_SESSION = 214;
	public final static int OP_CODE_VIDEOCALL_PIP = 215;
	public final static int OP_CODE_VIDEOCALL_UNMUTE = 216;
	
	
	public final static int OP_CODE_SIP_TRIGGERED_ENTER_SESSION = 298;
	public final static int OP_CODE_SIP_TRIGGERED_ACT = 299;
	
	public final static int OP_CODE_PHONEBOOK_EDIT = 301;
	public final static int OP_CODE_PHONEBOOK_ADD = 302;
	public final static int OP_CODE_PHONEBOOK_DELETE = 303;
	
	public final static int OP_CODE_RESULT = 310;
	public final static int OP_CODE_Return_to_Homepage = 311;
	public final static int OP_CODE_SETTING = 320;
	public final static int OP_CODE_CH_UPDATE = 350;
	
	public final static int OP_CODE_PX_BYE_TO_SR = 404;
	
	public final static int OP_CODE_SMARTCONNECT_FAIL = 700;
	public final static int OP_CODE_SMARTCONNECT_SUCCESS = 701;
	public final static int OP_CODE_SMARTCONNECT_CONNECTING = 702;
	public final static int OP_CODE_SMARTCONNECT_RESULT = 703;
	public final static int OP_CODE_SMARTCONNECT_RESULT_LIST = 704;
	public final static int OP_CODE_SMARTCONNECT_SCANNING_FINISH = 705;
	public final static int OP_CODE_SMARTCONNECT = 799;
	public final static int OP_CODE_SMARTCONNECT_FROM_QT_DIALER = 798;
	public final static int OP_CODE_SMARTCONNECT_FROM_QT_CALL_SESSION = 797;
	
	public final static int OP_CODE_PROVISION_CHECK = 500;
	public final static int OP_CODE_PROVISION_DOWNLOAD = 501;
	public final static int OP_CODE_PHONEBOOK_CHECK = 502;
	public final static int OP_CODE_PHONEBOOK_DOWNLOAD = 503;
	
	
	
	public final static int ACTIVITY_CODE_SR2_DIALER = 2010;
	public final static int ACTIVITY_CODE_SR2_CONTACTS = 2020;
	public final static int ACTIVITY_CODE_SR2_HISTORY = 2030;
	public final static int ACTIVITY_CODE_SR2_OUTCALL = 2040;
	public final static int ACTIVITY_CODE_SR2_INCALL = 2041;
	public final static int ACTIVITY_CODE_SR2_INSESSION = 2042;
	
	
	public final static int OP_CODE_KEEP_ALIVE = 900;
	public final static int OP_CODE_KEEP_ALIVE_ACK = 901;
	
	public final static int ACTIVITY_ADVANCE_CALL = 990;
	public final static int ACTIVITY_PB_EDIT= 999;
	public final static int ACTIVITY_QT_SETTING_CONNECT_RESULT= 998;
	
	
	public final static int RESULT_OK = 900;
	public final static int RESULT_FAIL = 901;
	
	public final static int PICK_RINGTONE = 100;
	public final static int MAKE_CALL_ORIGIN_DIALPAD = 1;
	public final static int MAKE_CALL_ORIGIN_PHONEBOOK = 2;
	public final static int MAKE_CALL_ORIGIN_CALLLOG = 3;
	public final static int CALLMODE_VOICE = 1;
	public final static int CALLMODE_VIDEO = 2;
	public final static int VOLUME_UP = 7;
	public final static int VOLUME_DOWN = 8;
	public final static int LOUDSPEAKER_ON = 9;
	public final static int LOUDSPEAKER_OFF = 10;
	
	public final static int SOUND_PORT_INIT = 9900;
	public final static int SOUND_PORT_LIMIT = 10000;
	public final static int SOUND_PACKET_SIZE = 640;
	public final static int SOUND_PACKET_SIZE_IN_SHORT = 320;
	
	public final static int AUDIOTRACK_PLAY = 1;
	public final static int AUDIOTRACK_STOP = 0;
}
