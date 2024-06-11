/*
Quanta QRI Copyright (C) 2009 

This is VC Application Software Header file.
*/

#ifndef ITRI_DEFINE_H
#define ITRI_DEFINE_H

#define IN 
#define OUT

#define QU_OK 							1

#define SERVERPORT 						5060

#define LOCALHOST 						"local"
#define LOCALPORT 						5060
#define LOCALIP 						"127.0.0.1"

#define REGISTER_EXPIRE 				"120"

#if (defined PX2) || (defined X86)
	#define NUMOFCALL 					3/*1~20 */
#else
	#define NUMOFCALL 					2/*1~20 */
#endif

#define NUMOFREGISTER 					2

#define RESULT_INVITE_PROGRESS		-2
#define RESULT_INVITE_INSESSION		-3

//#define Q_PRODUCT "Quanta PX2_0.0.1"

#define COMPANY 						"Quanta"
#define PROJECT_ET1 					"ET1"
#define PROJECT_QT1A 					"QT1.5A"
#define PROJECT_QT1W 					"QT1.5W"
#define PROJECT_QT1I 					"QT1.5I"
#define PROJECT_PX1 					"PX1"
#define PROJECT_PX2 					"PX2"
#define PROJECT_MCU 					"MCU"
#define PROJECT_PTT 					"PTT"

/*Audio Codec Name*/
#define PCMU_NAME 						"g711u"
#define G7221_NAME 						"g7221"
#define AAC_LC_NAME 					"aac_lc"
#define PCMA_NAME 						"g711a"
#define G722_NAME 						"g722"
#define OPUS_NAME 						"opus"


#define H264_NAME 						"H264"
#define Q_H264_NAME 					"Q_H264"

#define H264PAYLOAD 					"109"

#define FEC_NAME 						"red"
#define AUDIO_FEC_PAYLOAD 				"98"
#define AUDIO_FEC 						1
#define VIDEO_FEC_PAYLOAD 				"121"

#define RFC4588_NAME 					"rtx"
#define VIDEO_RFC4588_PAYLOAD 			"97"

#define H264PROFILELEVEL 				"42E00D"//QVGA 13
#define H264PACKETMODE 					"0"

//#define H264_CT_BANDWIDTH 				"800"

#define Q_H264PAYLOAD 					"111"

#define LAUDIOPORT 						9000
#define LVIDEOPORT 						9200
#define SESSIONEXPIRES 					60
#define EXPIRES 						"60"

#define SIPREALM 						"ICLSIP"

#define QSTREAM 						1
#define FW								1
#define SIP 							1

#define SRV 							1
#define IMS_SYSTEM 						0
#define CHALLENG_REQUEST 				1
#define CHECK_SOURCE 					1

#define check_answer_complete 			1

#define THREE_WAY_VIDEO_CONFERENCE 		0

#define CONFIG_SERVER 					"http://71.243.124.11/Quanta"
#define CONFIG_NAME 					"config.cfg"
#define CONFIG_SAVE_PATH 				"/home/nicky"
#define CONFIG_OUTPUT_FILE 				"config.abc"

#define debug_printf() 					printf("[debug]%s,%d\n",__FILE__,__LINE__)


#define NUMOFAUDIOCODEC 				3 //ebable opus , disable g722 g7221
#define CODEC_G722 						1

#define ENABLE_PCMA 					1

#define AAC 							0


#define SRV_TIMEOUT 					3
#define TIMER_B_TIMEOUT 				32

#define EARLY_MEDIA_SESSION 			11

#define CM 								0

#define CALLCONTROL_LOG 				0

#define TEST_INFO_CONTROL 				1

#define PREINVITE_TEST 					1
#define PREINVITE_CALLEE_TEST 			1

#define QUANTA_CODEC 					0

#define DEFAULT_CONFIG_NAME 			"framework.cfg"

//#define USER_AGENT_NAME "Quanta_SIP_V_001" 

//#define H264_CT_BANDWIDTH 2048
//#define H264_TIAS_BANDWIDTH 2048000

#define FORCE_LEVEL_ID 					0

#define LOCAL_TEST 						1

#define NOT_NEED_ACK 					-1

#define SR1

#define SESS_BUFF_LEN 					4096

#define H264_PK_MODE 					0
#define PRE_RX_SESSION 					1


#define PRINT_PACKET 					0

//#define ITRI_RESULT_OK 1
//#define ITRI_RESULT_FAIL 0

typedef enum {
	NONE_QUANTA=0,
	ET1,
	QT1A,
	QT1W,
	QT1I,
	QUANTA_PX2,
    QUANTA_3G = 6,
    QUANTA_MCU,
    QUANTA_PTT,
	QUANTA_OTHERS = 100,
}QUANTA_PRODUCT_TYPE;

/*
 * type attributes for audio codec
 */
typedef enum {
	PCMU=0,
	G7221,
	PCMA,
	G722,
	AAC_LC,
	OPUS,
	NUM_OF_AUDIO_CODEC,
}AUDIO_CODEC_E;

typedef enum {
	H264=0,
	H263
}VIDEO_CODEC;

#endif


/* Support SIP Request Methods 
typedef enum {
    INVITE=0,
    ACK,
    OPTIONS,
    BYE,
    CANCEL,
    REGISTER,  //RFC 2543 standard methods
    SIP_INFO,      //RFC 2976 standar methods
    PRACK,     // RFC 3262 
    //COMET,    due to being expired
    REFER,      //RFC 3515 
    // new methods for simple api
    SIP_SUBSCRIBE,
    SIP_NOTIFY,                                                                                                                                                                                                                               
    SIP_PUBLISH,
    SIP_MESSAGE,
    
    SIP_UPDATE,     // RFC 3311 
    UNKNOWN_METHOD // unknown method 
} SipMethodType;

typedef enum {
        CALL_STATE_INITIAL=0,
        CALL_STATE_PROCEED,
        CALL_STATE_RINGING,
        CALL_STATE_ANSWERED,
        CALL_STATE_CONNECT,
        CALL_STATE_OFFER,
        CALL_STATE_ANSWER,
        CALL_STATE_DISCONNECT,
        CALL_STATE_NULL
}ccCallState;

typedef enum{
        MEDIA_AUDIO=0,
        MEDIA_VIDEO,
        MEDIA_APPLICATION,
        MEDIA_DATA,
        MEDIA_MESSAGE
}MediaType;

*/
