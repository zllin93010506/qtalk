/*
Quanta QRI Copyright (C) 2009 

This is VC Application Software Header file.
*/

#ifndef CALLMANAGER_H
#define CALLMANAGER_H

#include <callcontrol/callctrl.h>
#include <pthread.h>

#include "icall_engine.h"
#include "itri_define.h"
#include "itri_call.h"
#include "srtp.h"

namespace call{class CallEngineFactory;};


typedef enum {
	EMPTY=0,
	HOLD,
	BEHOLD,
	HOLD_BEHOLD,
	ACTIVE
	
}QuCallState;

typedef enum {
	AUDIO=0,
	VIDEO
	
}QuMediaType;

typedef enum {
	UNATTEND_TRANSFER=1,
	ATTEND_TRANSFER,
}QuTransferType;

typedef enum {
	RESPONSE_EMPTY=0,
	RESPONSE_SWITCH_TO_AUDIO,
	RESPONSE_SWITCH_TO_VIDEO,
	
}QuResponseMode;

typedef enum {
	REQUEST_EMPTY=0,
	REQUEST_HOLD,
	REQUEST_UNHOLD,
	REQUEST_SWITCH_TO_AUDIO,
	REQUEST_SWITCH_TO_VIDEO
	
}QuRequestMode;

typedef enum {
	NO_RINGING=0,
	LOCAL_RINGING,
	EARLY_MEDIA	
	
}QuRingBackType;

typedef enum{
	NORMAL=0,
	PRE_TRANSFER,
	AFTER_TRANSFER,
	AFTER_FORWARD,
}QuCallFlowStatus;

typedef enum{
	CONF_ABSENSE=0,
	CONF_HELD,
	CONF_JOINING,
	CONF_ACTIVE
}QuCallConferenceParticipantStatus;


typedef struct remote_struct{
	int audio_port;
	int video_port;
	char *ip;
}Remote;
/*
typedef enum{
	WAITREQUEST_EMPTY=0,
	WAITREQUEST_HOLD,
	WAITREQUEST_RESUME,
}QuWaitRequest;
*/
typedef enum{
	WAITRESPONSE_EMPTY=0,
	WAITRESPONSE_HOLD,
	WAITRESPONSE_RESUME,
}QuWaitResponse;

typedef struct call_struct{
    char buffer[128];
	itricall::ItriCall* qt_call;
	pCCCall call_obj;
	pSess	lsdp;
	pSess	rsdp;
	char display_name[64];
	char phone[64];
	char referphone[64];
	int session;
	int callstate;
	/*bool isrefer;
	bool isafterrefer;
	bool isafterforward;
	The above three are replaced by "flow_status"*/
	QuCallFlowStatus flow_status;

	pCCCall refer_obj;
	pSipDlg refer_dlg;
	int media_type;
	int dtmf_transmit;
	
	int audio_port; //local audio_port
	int video_port; //local video_port
	//for re-invite
	char *reqid;
	pSipDlg dlg;
	
	char* callid;
	
	int isquanta;
	int ring_back_type;
	
	int audio_codec;
	int video_codec;
	
	char sip_sendip[32];
	
	int request_mode;
	int response_mode;
	//bool isSwitchMediaMode;
	//int wait_request; //follows QuWaitRequest
	int wait_response; //follows QuWaitResponse
	//bool send_prack;
	int num_of_wait_implicit_calls;//for video conference
	QuProfileLevelID remote_profile_level_id; //2011-05-26

	int multiple_way_video;
	bool make_implicit_call;
	
	char *conference_URI;
#ifdef SR1	
	/*2011-0704*/
	/*0:dect 1:smart phone*/
	int audio_source;
#endif

	bool sip_terminate_call;
	bool is_rev_ack;
    
	bool thrd_transfer_notify_expire;
	
	BOOL g_RequestProgress;
    
    int transfer_mode;
	
    char cp_phone[3][64]; // confernence participants
}QuCall;

typedef struct reg_struct{
	pCCCall regcall;
	SipReq regreq;
	SipRsp regrsp;
	pCCUser user;
	
	int expires;
	BOOL startkplive;
	int is_register;
	
	pthread_t threadRegister;
	int re_transmit;
	unsigned int handle_cseq; // store the handled CSeq
	
}QuRegister;

typedef struct profile{
	char proxy_server[128];//"viperdevas01.walvoip.net";
	char outbound_server[128];// "sip.vzalpha.com";
	char realm[128]; //"viperdevas01.walvoip.net";
	char sipid[64]; // 7817689678  ,x1020
	char username[64]; // 7817689678  ,1020
	char password[64]; //quanta
	char ip[32]; //"192.168.100.105"
}QuProfile;

typedef struct expire{
	int expires;
	char phone[64];
	char displayname[64];
}QuSessexpire;

typedef struct audio_codec{
	/*
	char *payload;

	char *codec;

	char *sample_rate;

	char *fmap;
	*/
	char payload[8];
	char codec[16];
	char sample_rate[8];
	char fmap[64];

}QuAudioCodec;

typedef struct _QTState{
	int dnd;
}QTState;

/*typedef enum {
  LEVEL_1_0=10,
  LEVEL_1_1,
  LEVEL_1_2,
  LEVEL_1_3,
  LEVEL_2_0=20,
  LEVEL_2_1,
  LEVEL_2_2,
  LEVEL_3_0=30,
  LEVEL_3_1,
  LEVEL_3_2,
  LEVEL_4_0=40,
  LEVEL_4_1,
  LEVEL_4_2,
  LEVEL_5_0=50,
  LEVEL_5_1,
}QuantaLevelIdType;
define in stream.manager.h*/

typedef struct Participant{
  char phone[64];
  QuCallConferenceParticipantStatus status;
  bool isNeedNotify;
}QuParticipant;


struct sessmediaObj{
	MediaType type;
	char *address;
	UINT32 port;
	UINT16 numofport;
	char* transport;
	UINT32 ptime;
	StatusAttr status;
	DxLst	rtpmaplist;
	DxLst	fmtplist;
	DxLst	bandwidthlist;
	DxLst	attribelist;

	/* for sRTP */
	DxLst	cryptolist;
};

struct Attobj{
	char*	name;
	char*	value;
};
typedef struct Attobj* pMediaAtt;
/*
bool_t is_enum(const char *sipaddress, char **enum_domain);
int enum_lookup(const char *enum_domain, enum_lookup_res_t **res);
void enum_lookup_res_free(enum_lookup_res_t *res);
*/
	//void *thread_function_Events(void *arg);
	//void *CallerExpireThread(void *arg);
	//void *CalleeExpireThread(void *arg);
	//void *kpliveThread(void *arg);
	
	bool GetStrDataFromFile(char *filename, char *field, char *str_ptr);
	
	int ap_init();	
	void getPhoneFromReq(SipReq req, char *phone);
	void getPhoneToReq(SipReq req,char *phone);
	void getPhoneFromRsp(SipRsp rsp, char *phone);
	void getPhoneToRsp(SipRsp rsp,char *phone);

	void GenAudioMedia(pSessMedia *media );
	void GenVideoMedia(pSessMedia *media );
	void GenVideoMediaByOriPort(pSessMedia *media,int id );
	
	//pSess QuContToSess(pCont content);
	void QuContToSessToStr(pCont content , char *sessbuf ,UINT32 *sesslen);

	pSess QuOfferToAnswer(IN pSess offer);
	int QuOfferToAnswerToStr(int call_id, pSess offer,char *sessbuf ,int *sesslen);
	void QuNewOfferToAnswerToStr(pSess offer,int id, char *sessbuf, int *sesslen);
	void QuReInviteOfferToAnswer(int id , pSess offer);
	int QuVideoFmtpAnswer(int call_id, pMediaPayload remote_payload,const char *payloadtype,pMediaPayload *local_payload);
	int QuParseProfileLevelID(pMediaPayload payload,QuProfileLevelID *profile_level_id);
	int QuPrintProfileLevelID(const QuProfileLevelID *profile_level_id);
				   
	void QuStartCallerExpireThread(char *addr);
	//void QuStartCalleeExpireThread(char *displayname,char *addr ,int expires);
	int QuGetFreeCall();
	int QuGetFreeSession();
	int QuGetCall(char *addr);
    bool QuCheckInSession();
    int QuGetCallFromICall(itricall::ItriCall* qt_call);
	//int QuFreeCall(int callid);
    int QuFreeCall(int callid,bool is_need_notify ,const char *message);
	int QuGetCallBySession(int session); //2011-05-24
	
	int QuGetConfInfo(const char* addr, char* conference_info);
	int QuMakeCallEx(unsigned int mode, char* addr, SipHdrType hdrtype, const char* hdrvalue);
	int QuHoldCall(char *addr);
	int QuMusicHoldCall(char *addr);
	int QuUnHoldCall(char *addr);
	int QuSwitchCallMode(int mode ,char *addr);
	//int QuPreAnswerCall(pCCCall call, SipReq preq);
	int QuPreAnswerCall(pCCCall incomecall ,SipReq preq ,pSipDlg callleg);
	int QuRevBye200OK(SipRsp rsp);
	int QuRevBye(SipReq req);
	int QuRecCancel(SipReq req);
	int QuRevReInvite(pCCCall call, SipReq req, pSipDlg dlg);
	int QuRevReInvite200OK(pCCCall call, SipRsp rsp);
	int QuReferMakeCall(char *addr,char *raddr ,SipReq preq ,int audio_source);
	int QuRevAck(SipReq req);
	int QuRevReAck(SipReq req);
	int QuRevCancel(SipReq req);
	int QuInviteFail(SipRsp rsp);
	int QuCancelTimeout(int id);
	int QuByeTimeout(SipReq req);
	int QuInviteTimeout(SipReq req);
	
	const char* QuConfStatusToString(QuCallConferenceParticipantStatus st);
	int QuStringToConfStatus(const char * string,QuCallConferenceParticipantStatus* status);
		
	int QuSetIsQuantaProduct(int session, pSess sess);
	//void QuUpdateNumberOfParticipant();
	int QuSetIsVideoConference(int callid, pSess sess, char* str);
	int QuRemoveParticipant(char *addr);
	int QuConfStatusUpdateByString(const char* phonenumber,const char* info);
	int QuCleanupConference();
	int QuGetMultipleWayVideo(int mode);
	int QuNotifyParticipantToFW(unsigned int mode);
	
	bool QuIsIFrameRequestCheck(const char* sdp);
	
	const char* QuLevelIdToString(int level_idc); //2011-05-23
	int QuVideoConferenceHold(char* addr);
	int QuVideoConferenceUnHold(char* addr);
	int QuVideoConferenceStartNotice(char* addr) ;
	
	int QuNewInfoDTMF(char *addr, char event,int duration);
	
	int check_answer_completely(pSess sess);
	const char* confStatusToString(QuCallConferenceParticipantStatus st);
	
	int GetRefresher(SipSessionExpires *se);
	int QuRevUpdate(pCCCall call_obj, SipReq req, pSipDlg dlg);
	
	int QuParseH264Parameter(pMediaPayload payload,QuH264Parameter *h264_parameter);
	int QuParseH264LevelID(pMediaPayload payload,QuH264Parameter *h264_parameter);
	
	int GetVideoBandwidth(pSess sess);
	
	void wait_ack(int timeout);
	char* telURLGetIMPU(char *url);
	char* sipURLGetIMPU(char *url);
    char* sipURLGetDomain(char *url);

	void *QuCancelExpireThread(void *arg);
	void *QuAckExpireThread(void *arg);

	void MediaCodecInfoInit(MediaCodecInfo *media_info);
	//Please DO NOT pass a local pointer to the thread
	void *reinvite_codec(void *arg);
	void *reinvite_switch_network_interface(void *arg);

	int check_incoming_call_mode (pSess sess);
	void MediaCodecInfoInit(MediaCodecInfo *media_info);
	
	void QTSetIsQuantaProduct(int call_id);
    void QuStartTransferNotifyExpireThread(char *addr);

    int QuSetFec( char*match_payloadtype, pSess sess);
    int GetSipFrag(pCont content);
    void *QuAddConferenceParticipants(void *arg);

#endif
