#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include   <windows.h>
	#include "DbgUtility.h"
	#define printf dxprintf
	#define strcasecmp _stricmp
#else
	#include <unistd.h>
	#include <netdb.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
	#include <pthread.h>
	#define _snprintf snprintf
#endif

#include "itri_call.h"
#include "itri_sip.h"

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif
#include <sipTx/sipTx.h>
#include "callmanager.h"
#include "itri_define.h"
#include "itri_utility.h"
#include "stream_manager.h"
#include "srtp.h"

#ifdef _ANDROID_APP
#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#define printf LOGD
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include "sip_log_ios.h"
#define printf CLog_sip

#endif

using namespace call;


extern call::ICallEventListener* g_event_listner;

int local_audio_port = LAUDIOPORT;
int local_video_port = LVIDEOPORT;

extern bool g_inviteProgress;

extern QuAudioCodec audio_codec[NUM_OF_AUDIO_CODEC];
extern int media_bandwidth;

extern pthread_t threadExpire;

extern QuCall qcall[NUMOFCALL];
extern Config sip_config;
extern QuProfile profile;
extern QuRegister reg;
extern QTState qt_state;

extern bool g_prack_enable;

extern char g_product[80];

extern int enable_srtp;
extern char outbound_url[256];
extern int g_local_listen_port;

/*
	Check uhhold or transfer for Cisco
*/
int is_cisco_transfer=0;

void GenAudioMedia(pSessMedia *media )
{

	char audio_priority[NUMOFAUDIOCODEC][16];
	
	int priority[NUMOFAUDIOCODEC];
	
	memset(audio_priority[0],0,sizeof(audio_priority[0]));
	strcpy(audio_priority[0],sip_config.qtalk.sip.audio_codec_preference_1);
	//strcpy(audio_priority[0],OPUS_NAME);

	memset(audio_priority[1],0,sizeof(audio_priority[1]));
	strcpy(audio_priority[1],sip_config.qtalk.sip.audio_codec_preference_2);
	//strcpy(audio_priority[1],PCMU_NAME);
		
	memset(audio_priority[2],0,sizeof(audio_priority[2]));
	strcpy(audio_priority[2],sip_config.qtalk.sip.audio_codec_preference_3);
	//strcpy(audio_priority[2],PCMA_NAME);

	//memset(audio_priority[3],0,sizeof(audio_priority[3]));
	//strcpy(audio_priority[3],G722_NAME);

	printf("audio codec 1=%s\n",sip_config.qtalk.sip.audio_codec_preference_1);
	printf("audio codec 2=%s\n",sip_config.qtalk.sip.audio_codec_preference_2);
	printf("audio codec 3=%s\n",sip_config.qtalk.sip.audio_codec_preference_3);

	int i;
	
	for(i=0;i<NUMOFAUDIOCODEC;i++){
		
		if (!strcasecmp(audio_priority[i],PCMU_NAME)){
			priority[i]=PCMU;
		}else if (!strcasecmp(audio_priority[i],G7221_NAME)){
			priority[i]=G7221;
		}
		else if (!strcasecmp(audio_priority[i],PCMA_NAME)){
			priority[i]=PCMA;
		}
		else if (!strcasecmp(audio_priority[i],G722_NAME)){
			priority[i]=G722;
		}else if (!strcasecmp(audio_priority[i],OPUS_NAME)){
			priority[i]=OPUS;
		}else{
			priority[i]= -1;
		}
	}	
	
	pMediaPayload payload=NULL;
	
	for(i=0;i<NUMOFAUDIOCODEC;i++){
		
		if (0==i){
			printf("priority[i]=%d\n",priority[i]);
			printf("i=%d %s,%s\n",i,audio_codec[priority[i]].payload,audio_codec[priority[i]].codec);
			MediaPayloadNew(&payload,audio_codec[priority[i]].payload,audio_codec[priority[i]].codec);
			SessMediaNew(media,MEDIA_AUDIO,NULL,local_audio_port,"RTP/AVP",20,RTPMAP,payload);
		}else{
			printf("i=%d %s,%s\n",i,audio_codec[priority[i]].payload,audio_codec[priority[i]].codec);
			if (priority[i] != -1){
				MediaPayloadNew(&payload,audio_codec[priority[i]].payload,audio_codec[priority[i]].codec);	
				SessMediaAddRTPParam(*media ,RTPMAP,payload);
			}
		}
		
		if (priority[i]==G7221){
			char g7221_bitrate[32];
			snprintf(g7221_bitrate,32*sizeof(char),"bitrate=%s",audio_codec[G7221].fmap);
			MediaPayloadNew(&payload,audio_codec[G7221].payload,g7221_bitrate);
			SessMediaAddRTPParam(*media ,FMTP,payload);
		}else if (priority[i]==OPUS){
			char opus_fmtp[256];
			snprintf(opus_fmtp,256*sizeof(char),"maxplaybackrate=%d;sprop-maxcapturerate=%d;maxaveragebitrate=%d;"
					"stereo=%d;useinbandfec=%d",16000,16000,24000,0,1);
			MediaPayloadNew(&payload,audio_codec[OPUS].payload,opus_fmtp);
			SessMediaAddRTPParam(*media ,FMTP,payload);
		}
	}
	//????MediaPayloadNew(&payload,audio_codec[priority[0]].payload,audio_codec[priority[0]].codec);	
	
	//MediaPayloadNew(&payload,"121","bitrate=24000");
#if AUDIO_FEC
    MediaPayloadNew(&payload,AUDIO_FEC_PAYLOAD,"red/90000");
    SessMediaAddRTPParam(*media ,RTPMAP,payload);
    char rtpmap[256];
    memset(rtpmap,0,256*sizeof(char));
    for(i=0;i<NUMOFAUDIOCODEC;i++){
        if (0==i){
            snprintf(rtpmap,256*sizeof(char),"%s",audio_codec[priority[i]].payload);
        }else{
            if (priority[i] != -1){
                snprintf(rtpmap,256*sizeof(char),"%s/%s",rtpmap,audio_codec[priority[i]].payload);
            }
        }
    }
    
    MediaPayloadNew(&payload,AUDIO_FEC_PAYLOAD,rtpmap);
    SessMediaAddRTPParam(*media ,FMTP,payload);
    
#endif
    
	SessMediaAddAttribute(*media,"tool:",g_product);//a=tool:qsip v.0.0.1
	
	//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
	{
		MediaPayloadNew(&payload,"101","telephone-event/8000");
		SessMediaAddRTPParam(*media ,RTPMAP,payload);
	}
	
	//enable SRTP
	char srtp_key[256];
	if (enable_srtp){
		snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLER_AUDIO_SRTP_KEY);
		SessMediaAddCrypto(*media , srtp_key);
	}
	//SessMediaAddAttribute(*media,"sendrecv",NULL);

}

extern struct call::MDP    local_video_mdp;

void GenVideoMedia(pSessMedia *media )
{
	pMediaPayload payload=NULL;
	
	char str_info_ctrl[32]="0";
	
	MediaPayloadNew(&payload,H264PAYLOAD,"H264/90000");
	SessMediaNew(media,MEDIA_VIDEO,NULL,local_video_port,"RTP/AVP",20,RTPMAP,payload);

	//SessMediaNew(media,MEDIA_VIDEO,NULL,LVIDEOPORT,"RTP/AVP",20,RTPMAP,payload);
	
	/*2010-0511 h264 profile parameter*/
	char packetization[16]=H264PACKETMODE;
	char rtpmap[256]="0";
	/*
	char maxmbps[16]=H264MBPS;
	char maxfs[16]=H264MFS;
	char maxcpb[16]="0";
	char maxdpb[16]="0";
	char maxbr[16]="0";
    */
    
    snprintf(rtpmap,256*sizeof(char),"profile-level-id=42E0%02X",local_video_mdp.level_idc);
    
	snprintf(rtpmap,256*sizeof(char),"%s;packetization-mode=%s",rtpmap,packetization);
	//printf("%s\n",rtpmap);

	if (local_video_mdp.max_mbps > 0){
		snprintf(rtpmap,256*sizeof(char),"%s;max-mbps=%d",rtpmap,local_video_mdp.max_mbps);
	}
	//printf("%s\n",rtpmap);
	if (local_video_mdp.max_fs > 0){
		snprintf(rtpmap,256*sizeof(char),"%s;max-fs=%d",rtpmap,local_video_mdp.max_fs);
	}
	
	printf("%s\n",rtpmap);
	//printf("rtpmap\n%s\n",rtpmap);
	MediaPayloadNew(&payload,H264PAYLOAD,rtpmap);
	SessMediaAddRTPParam(*media ,FMTP,payload);
    
    //if (!strcmp(sip_config.generic_rtp.video_fec_enable,"1")){
    if(1){
		MediaPayloadNew(&payload,VIDEO_FEC_PAYLOAD,"red/90000");
		SessMediaAddRTPParam(*media ,RTPMAP,payload);
		
		memset(rtpmap,0,256*sizeof(char));
		snprintf(rtpmap,256*sizeof(char),"%s",H264PAYLOAD);
		MediaPayloadNew(&payload,VIDEO_FEC_PAYLOAD,rtpmap);
		SessMediaAddRTPParam(*media ,FMTP,payload);
		
	}
    
	/*2011-06-24 add bandwidth value*/
	int tias_bandwidth = media_bandwidth * 1000;
	SessMediaAddBandwidth(*media,"TIAS",tias_bandwidth);
	
	SessMediaAddAttribute(*media,"rtcp-fb",":* nack pli");//a=rtcp-fb:109 nack pli
	SessMediaAddAttribute(*media,"rtcp-fb",":* ccm fir");//a=rtcp-fb:109 ccm fir

	//enable SRTP
	char srtp_key[256];
	if (enable_srtp){
		snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLER_VIDEO_SRTP_KEY);
		SessMediaAddCrypto(*media , srtp_key);
	}
}

void *QuCancelExpireThread(void *arg){
	int *tmp = (int*)arg;
	int id = *tmp;
	free(tmp);
    
	int timer = 0;
	while (timer < 32 ){
		if (!qcall[id].sip_terminate_call)
		{
			break;
		}
        
		sleep(1);
		timer=timer+1;
		printf("CancelExpireThread id=%d qcall[id].sip_cancel_call=%d\n",id, qcall[id].sip_terminate_call);
		//timeout 32 sec;
	}
	
	if(timer >= 32)
	{
	    QuCancelTimeout(id);
	}
    
	return NULL;
}

#define SIP_ACK_TIMEOUT 5

void *QuAckExpireThread(void *arg){
    int *tmp = (int*)arg;
    int id = *tmp;
    free(tmp);
    
    int timer = 0;
    while (timer < SIP_ACK_TIMEOUT ){
        if (qcall[id].is_rev_ack)
        {
            break;
        }
        
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif;
        timer=timer+1;
        printf("ACKExpireThread id=%d qcall[id].sip_ack_call=%d ,timer=%d\n",id, qcall[id].is_rev_ack,timer);
        //timeout 32 sec;
    }
    
    if(timer >= SIP_ACK_TIMEOUT)
    {
        QuFreeCall(id,true,"BYE");
        g_inviteProgress=FALSE;
    }
    
    return NULL;
}


void *reinvite_codec(void *arg) {
	char sessbuf[SESS_BUFF_LEN]="0";
	UINT32 sess_len=SESS_BUFF_LEN-1 ;
	pCont	msgbody=NULL;
	pSessMedia audio_media=NULL;
	pSessMedia video_media=NULL;
	pMediaPayload payload=NULL;
	int media_size;
	int i;
	//2011-12-08 
	int *p_id = (int*) arg;
	int id = * p_id;
	free(p_id);
	
	g_inviteProgress=TRUE;
	
#ifdef _WIN32
	Sleep(1000);
#else
	sleep(1);
#endif

	printf("call id = %d\n",id);
	if (-1==id){
	    return 0 ;
	} else if (id > NUMOFCALL) {
	    printf("\n\n\n\n\n\n <sip> CALL ID ERROR in reinvite_codec.");
	    return 0;
	}
	
	MediaPayloadNew(&payload,audio_codec[qcall[id].audio_codec].payload,audio_codec[qcall[id].audio_codec].codec);
	SessMediaNew(&audio_media,MEDIA_AUDIO,NULL,qcall[id].audio_port,"RTP/AVP",20,RTPMAP,payload);
	
	if (G7221 == qcall[id].audio_codec){
		char g7221_bitrate[32]="0";
		snprintf(g7221_bitrate,32*sizeof(char),"bitrate=%s",audio_codec[G7221].fmap);
		MediaPayloadNew(&payload,audio_codec[G7221].payload,g7221_bitrate);
		SessMediaAddRTPParam(audio_media ,FMTP,payload);
	}else if (OPUS == qcall[id].audio_codec){
		char opus_fmtp[256];
		snprintf(opus_fmtp,256*sizeof(char),"maxplaybackrate=%d;sprop-maxcapturerate=%d;maxaveragebitrate=%d;"
				"stereo=%d;useinbandfec=%d",16000,16000,24000,0,1);
		MediaPayloadNew(&payload,audio_codec[OPUS].payload,opus_fmtp);
		SessMediaAddRTPParam(audio_media ,FMTP,payload);
	}
	
	SessMediaAddAttribute(audio_media,"tool:",g_product);//a=tool:qsip v.0.0.1

	if(1){
	//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
		MediaPayloadNew(&payload,"101","telephone-event/8000");
		SessMediaAddRTPParam(audio_media ,RTPMAP,payload);
	}
	
	if (qcall[id].media_type==VIDEO){
		GenVideoMedia(&video_media);
	}
	
	//clear old sdp
	SessGetMediaSize(qcall[id].lsdp,&media_size);
	for (i=0;i<media_size;i++){
		SessRemoveMedia(qcall[id].lsdp,0);
	}
	
	//add new audio media
	SessAddMedia(qcall[id].lsdp,audio_media);
	
	if (NULL!=video_media){//add new video media
		SessAddMedia(qcall[id].lsdp,video_media);
	}
	SessAddVersion(qcall[id].lsdp);
    
    /*set new address */
    SessSetAddress(qcall[id].lsdp, profile.ip);
    SessSetOwnerAddress(qcall[id].lsdp, profile.ip);
    
	if (SessToStr(qcall[id].lsdp,sessbuf,(INT32*)&sess_len)==RC_OK) //output string
        ;//printf("****************\nsess sdp:\n%s\n",sessbuf);
	else printf("sess sdp fail\n");

	ccContNew(&msgbody,"application/sdp",sessbuf);

	printf("set outbound_url =%s\n",outbound_url);
	if (RC_OK== ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port)){
		printf("ccCallSetOutBound ok\n");
	}

	if (RC_OK==ccModifyCall(&qcall[id].call_obj,msgbody)){
		printf("ccModifyCall OK\n");
	}else{
		printf("ccModifyCall Fail\n");
		g_inviteProgress=FALSE;
	}
	ccContFree(&msgbody);
	
	printf("End reinvite_codec\n");

    return NULL;
}

int check_answer_completely (pSess sess){
	
	int media_size;
	int i,j;
	UINT32 rtpmap_size;
	pSessMedia media=NULL;
	pMediaPayload payload=NULL;
	MediaType media_type;
	int audio_codec_count=0;
	int video_codec_count=0;
	char payloadbuf[256]="0";
	int payloadlen=255;
	
	char payloadtype[32]="0";
	int payloadtypelen;
	
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			printf("MEDIA_AUDIO : %d\n",rtpmap_size);
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					payloadlen=255;
					payloadtypelen=31;
                    
					MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
					MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
					//char *slash=strchr(payloadbuf,'/');
					char payloadtmp[40]="0";
					strcpy (payloadtmp,payloadbuf);
					char *pch = strtok (payloadtmp,"/");
					int audio_type = atoi(payloadtype);
					if (0==audio_type ||  8==audio_type || 9==audio_type ){
						audio_codec_count++;
					}else if (strcasecmp(pch,"telephone-event") && strcasecmp(pch,FEC_NAME) && strcasecmp(pch,RFC4588_NAME)){
						audio_codec_count++;
					}
				}
			}
			if (1<audio_codec_count)
				return 1;
		}else if(media_type==MEDIA_VIDEO) {
			printf("MEDIA_VIDEO : %d\n",rtpmap_size);
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					payloadlen=255;
					payloadtypelen=31;
                    
					MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
					MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
					//char *slash=strchr(payloadbuf,'/');
					char payloadtmp[40]="0";
					strcpy (payloadtmp,payloadbuf);
					char *pch = strtok (payloadtmp,"/");
					int audio_type = atoi(payloadtype);
					
					if (strcasecmp(pch,"telephone-event") && strcasecmp(pch,FEC_NAME) && strcasecmp(pch,RFC4588_NAME)){
						video_codec_count++;
					}
				}
			}
			if (1<video_codec_count)
				return 1;
		}
	}
	return 0;
}

int QuGetFreeCall()
{
	int i=0;	
	for(i=0;i<NUMOFCALL;i++){
		if (qcall[i].call_obj==NULL)
		{
			return i;
		}
	}
	return -1;
}

bool QuCheckInSession()
{
	//int i=0;
	//for(i=0;i<NUMOFCALL;i++){
		if (qcall[0].call_obj != NULL && qcall[1].call_obj != NULL)
		{
			return true;
		}
	//}
	return false;
}

pthread_mutex_t qu_free_mutex=PTHREAD_MUTEX_INITIALIZER;

int QuFreeCall(int callid,bool is_need_notify ,const char *message)
{
	printf("QuFreeCall id=%d \n",callid);
	pthread_mutex_lock ( &qu_free_mutex );

	if(qcall[callid].qt_call != NULL)
	{
		if(g_event_listner != NULL){
            // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[callid].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
            pthread_mutex_lock(&qcall[callid].qt_call->mCallData.calldata_lock);
            usleep(10000);
			if(qcall[callid].qt_call->mCallData.is_session_start == true)
			{
				qcall[callid].qt_call->mCallData.is_session_start = false;
				printf("g_event_listner->onSessionEnd %d\n",qcall[callid].qt_call);
				g_event_listner->onSessionEnd(qcall[callid].qt_call);
			}

			qcall[callid].qt_call->mCallData.call_state = CALL_STATE_HANGUP;
			if (is_need_notify){
				printf("g_event_listner->onCallHangup %d\n",qcall[callid].qt_call);
				g_event_listner->onCallHangup(qcall[callid].qt_call,message);
			}
            // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[callid].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
            pthread_mutex_unlock(&qcall[callid].qt_call->mCallData.calldata_lock);
		}
	}
    if(qcall[callid].qt_call != NULL){
        delete qcall[callid].qt_call;
        qcall[callid].qt_call=NULL;
    }
	
	ccCallFree(&qcall[callid].call_obj);
	SessFree(&(qcall[callid].rsdp));
	SessFree(&(qcall[callid].lsdp));
	
	if(qcall[callid].reqid){
		free(qcall[callid].reqid);
	}

	//ccCallFree(&qcall[callid].refer_obj);
	/*
	qcall[callid].call_obj=NULL;	
	qcall[callid].lsdp=NULL;
	qcall[callid].rsdp=NULL;
	memset (&qcall[callid].phone,0,sizeof(qcall[callid].phone));
	qcall[callid].session =-1;
	qcall[callid].callstate = EMPTY;
	qcall[callid].flow_status = NORMAL;
	qcall[callid].media_type = 0;
	qcall[callid].dtmf_transmit = 0;
	memset (&qcall[callid].reqid,0,sizeof(qcall[callid].reqid));
	*/

	memset(&qcall[callid],0,sizeof(QuCall));
	qcall[callid].call_obj=NULL;
	qcall[callid].session =-1;
	
	pthread_mutex_unlock ( &qu_free_mutex );

	printf("END Free Call\n");
	return 1;	
}

int QuGetCall(char *addr)
{
	int i=0;	
	
	for(i=0;i<NUMOFCALL;i++){
	    /*2011-02-21 02-22*/
	    /* modify for the transfer-call behavior*/
	    //printf("qcall[%d].flow_status=%d phone=%s\n",i,qcall[i].flow_status,qcall[i].phone);
	    switch(qcall[i].flow_status)
	    {
	      case AFTER_FORWARD:
	      case AFTER_TRANSFER:
 		//printf("Call[%d]: phone:%s  referphone:%s\n",i,qcall[i].phone,qcall[i].referphone);
		if (!strcmp(qcall[i].phone,addr) || !strcmp(qcall[i].referphone,addr))
		{
			return i;
		}
		break;
	      case NORMAL:
	      case PRE_TRANSFER:
		//printf("Call[%d]: phone:%s,addr=%s\n",i,qcall[i].phone,addr);
		if (!strcmp(qcall[i].phone,addr))
		{
			return i;
		}
		
	    }

	}
	
	printf("[sip error]:QuGetCall can't get call id\n");
  	return -1;
}

void getPhoneFromReq(SipReq req,char *phone){
	
	SipURL fromurl=sipURLNewFromTxt(GetReqFromUrl(req)) ;
	char *tmp_char=NULL;

	char *from_url = GetReqFromUrl(req);
	if (from_url == NULL){
		printf("[getPhoneFromReq]to_url=NULL\n");
		strcpy(phone,"");
		return;
	}else{
		tmp_char = sipURLGetIMPU(from_url);
	}

	if (tmp_char!=NULL){
		//strcpy(phone,sipURLGetUser(fromurl));
		strcpy(phone,str_replace(tmp_char,"%23","#"));
	}else {
		//printf("[WARNING]sipURLGetUser NULL :%s,%d\n",__FILE__,__LINE__);
		tmp_char = telURLGetIMPU(GetReqFromUrl(req));
		printf("getPhoneFromReq=%s\n",tmp_char);
		if (tmp_char!=NULL){
			strcpy(phone,str_replace(tmp_char,"%23","#"));
		}
	}
	
	sipURLFree(fromurl);
	
}

void getPhoneToReq(SipReq req,char *phone){
	
	SipURL tourl=sipURLNewFromTxt(GetReqToUrl(req)) ;
	char *tmp_char=NULL;

	char *to_url = GetReqToUrl(req);
	if (to_url == NULL){
		printf("[getPhoneToReq]to_url=NULL\n");
		strcpy(phone,"");
		return;
	}else{
		tmp_char = sipURLGetIMPU(to_url);
	}
	
	if (tmp_char!=NULL){
		//strcpy(phone,sipURLGetUser(fromurl));
		strcpy(phone,str_replace(tmp_char,"%23","#"));
	}else {
		//printf("[WARNING]sipURLGetUser NULL :%s,%d\n",__FILE__,__LINE__);
		tmp_char = telURLGetIMPU(GetReqToUrl(req));
		printf("getPhoneToReq=%s\n",tmp_char);
		if (tmp_char!=NULL){
			strcpy(phone,str_replace(tmp_char,"%23","#"));
		}
	}
	
	sipURLFree(tourl);
	
}

void getPhoneToRsp(SipRsp rsp,char *phone){
	
	SipURL tourl=sipURLNewFromTxt(GetRspToUrl(rsp)) ;
	char *tmp_char=NULL;

	char *to_url = GetRspToUrl(rsp);
	if (to_url == NULL){
		printf("[getPhoneToRsp]to_url=NULL\n");
		strcpy(phone,"");
		return;
	}else{
		tmp_char = sipURLGetIMPU(to_url);
	}

	//str_replace(tmp_char,"%23","#");
	if (tmp_char!=NULL){
		//strcpy(phone,sipURLGetUser(tourl));
		strcpy(phone,str_replace(tmp_char,"%23","#"));
	}else {
		//printf("[WARNING]sipURLGetUser NULL:%s,%d\n",__FILE__,__LINE__);
		tmp_char = telURLGetIMPU(GetRspToUrl(rsp));
		printf("getPhoneToRsp=%s\n",tmp_char);
		if (tmp_char!=NULL){
			strcpy(phone,str_replace(tmp_char,"%23","#"));
		}
	}
	sipURLFree(tourl);
	
}

void getPhoneFromRsp(SipRsp rsp,char *phone){
	
	SipURL fromurl=sipURLNewFromTxt(GetRspFromUrl(rsp)) ;
	char *tmp_char=NULL;

	char *from_url = GetRspFromUrl(rsp);
	if (from_url == NULL){
		printf("[getPhoneFromRsp]from_url=NULL\n");
		strcpy(phone,"");
		return;
	}else{
		tmp_char = sipURLGetIMPU(from_url);
	}

	//str_replace(tmp_char,"%23","#");
	if (tmp_char!=NULL){
		//strcpy(phone,sipURLGetUser(fromurl));
		strcpy(phone,str_replace(tmp_char,"%23","#"));
	}else {
		//printf("[WARNING]sipURLGetUser NULL:%s,%d\n",__FILE__,__LINE__);
		tmp_char = telURLGetIMPU(GetRspFromUrl(rsp));
		printf("getPhoneFromRsp=%s\n",tmp_char);
		if (tmp_char!=NULL){
			strcpy(phone,str_replace(tmp_char,"%23","#"));
		}
	}
	sipURLFree(fromurl);
	
}

char* sipURLGetIMPU(char *url){
	
	char *pch=NULL;
	char *url_tmp;
	url_tmp = strdup(url);
	
	//printf("telURLGetUser url=%s\n",url_tmp);
	pch = strtok (url_tmp," :;");
	//printf("telURLGetUser pch=%s\n",pch);
	if (!strcmp("sip",pch)){
		pch = strtok (NULL," :;");
		printf("sipURLGetIMPU=%s\n",pch);
		
		//check domain
		char *url_tmp_domain;
		url_tmp_domain = strdup(pch);
		char *pch_domain=NULL;
		char *pch_user;
		pch_domain = strtok (url_tmp_domain," @");
		pch_user = strdup(pch_domain);
		pch_domain = strtok (NULL," ");
		
		if (pch_domain==NULL){
			return pch_user;
		}
		
		printf("pch_user=%s,pch_domain=%s\n",pch_user,pch_domain);
		//printf("proxy_server=%s\n",profile.proxy_server);
		if (1){
		//if (!strcmp(profile.proxy_server,pch_domain)){
			return pch_user;
		}else{
			return pch;
		}
		
	}else{
		return NULL;
	}
	
}

char* telURLGetIMPU(char *url){
	
	char *pch=NULL;
	char *url_tmp;
	url_tmp = strdup(url);
	
	//printf("telURLGetUser url=%s\n",url_tmp);
	pch = strtok (url_tmp," :;");
	//printf("telURLGetUser pch=%s\n",pch);
	
	if (pch==NULL){
		return NULL;
	}
	
	if (!strcmp("tel",pch)){
		pch = strtok (NULL," :;");
		printf("telURLGetIMPU=%s\n",pch);
		return pch;
	}else{
		return NULL;
	}
	
}

char* sipURLGetDomain(char *url){
	
	char *pch=NULL;
	char *url_tmp;
	url_tmp = strdup(url);
	
	//printf("telURLGetUser url=%s\n",url_tmp);
	pch = strtok (url_tmp," :;");
	//printf("telURLGetUser pch=%s\n",pch);
	if (!strcmp("sip",pch)){
		pch = strtok (NULL," :;");
		
		//check domain
		char *url_tmp_domain;
		url_tmp_domain = strdup(pch);
		char *pch_domain=NULL;
		char *pch_user;
		pch_domain = strtok (url_tmp_domain," @");
		pch_user = strdup(pch_domain);
		pch_domain = strtok (NULL," ");
		
        return pch_domain;
		
	}else{
		return NULL;
	}
	
}

int QuCancelTimeout(int id)
{

	printf("QuCancelTimeout id=%d\n",id);
	
	if (-1==id){
		return 0 ;
	}

	QuFreeCall(id,false,"");
	
	g_inviteProgress=FALSE;
	
	return 1;
}
//invite fail
int QuInviteFail(SipRsp rsp)
{
	int res=0;			
	char *displayname = GetRspToDisplayname(rsp);
	if (!displayname){
		displayname = strdup("");
	}
	char phonenumber[64]="0";
	getPhoneToRsp(rsp,phonenumber);
	
	int id = QuGetCall(phonenumber);
	//int id = QuGetCallFromCallid((char *)sipRspGetHdr(rsp,Call_ID));
	printf("QuInviteFail id=%d\n",id);
	
	if (-1==id){
		return 0 ;
	}
	char* reason = GetRspReason(rsp); // get reason by response
	if (!reason)
		reason = strdup("UnKnown");

	QuFreeCall(id,true,reason);
	
	g_inviteProgress=FALSE;
	
	return 1;
}
//invite timeout
int QuInviteTimeout(SipReq req)
{
	char *displayname = GetReqFromDisplayname(req);	
	if (!displayname){
		displayname=strdup("");
	}
	char phonenumber[64]="0";
	getPhoneToReq(req,phonenumber);
	
	
	int id = QuGetCall(phonenumber);
	//int id = QuGetCallFromCallid((char *)sipReqGetHdr(req,Call_ID));
	printf("QuInviteTimeout = %s,%d\n",phonenumber,id);
	
	if (-1==id){
		return 0 ;
	}

	QuFreeCall(id,true,"Invite TimeOut");
	
	g_inviteProgress=FALSE;
	
	return 1;
}
//Receive BYE 200ok
int QuRevBye200OK(SipRsp rsp)
{
	char phonenumber[64]="0";
	
	getPhoneToRsp(rsp,phonenumber);
	
	int id = QuGetCall(phonenumber);
	//int id = QuGetCallFromCallid((char *)sipRspGetHdr(rsp,Call_ID));
	
	if (-1==id){
		printf("[warning]:QuRevBye200OK id = -1;");
		return 0;
	}
	
	QuFreeCall(id,false,"");

	g_inviteProgress=FALSE;
	
	return 1;
	
}

int QuByeTimeout(SipReq req)
{
	
	char phonenumber[64]="0";
	getPhoneToReq(req,phonenumber);
	
	int id = QuGetCall(phonenumber);
	//int id = QuGetCallFromCallid((char *)sipReqGetHdr(req,Call_ID));
	
	printf("QuByeTimeout = %s,%d\n",phonenumber,id);
	
	if (-1==id){
		printf("[warning]:QuByeTimeout id = -1;");
		return 0;
	}
	
	QuFreeCall(id,false,"");
	
	g_inviteProgress=FALSE;
	return 1;
	
}

//Receive BYE
int QuRevBye(SipReq req)
{
	int res;			
	char *displayname = GetReqFromDisplayname(req);
	if (!displayname){
		displayname=strdup("");
	}
	char phonenumber[64]="0";
	getPhoneFromReq(req,phonenumber);
	
	int id = QuGetCall(phonenumber);
	printf("QuRevBye id=%d\n",id);

	if (-1==id){
		return 0 ;
	}
	
	QuFreeCall(id,true,"BYE");
	
	g_inviteProgress=FALSE;
	
	return 1;
}


//Receive CANCEL
int QuRevCancel(SipReq req)
{
				
	char *displayname = GetReqFromDisplayname(req);
	if (!displayname){
		displayname=strdup("");
	}
	char phonenumber[64]="0";
	getPhoneFromReq(req,phonenumber);
	
	printf("QuRevCancel\n");
	int id = QuGetCall(phonenumber);
    
	if (-1==id){
		return 0 ;
	}
	
	QuFreeCall(id,true,"Cancel Call");
	
	g_inviteProgress=FALSE;
	
	return 1;
}

void QuContToSessToStr(pCont content , char *sessbuf ,UINT32 *sesslen)
{
	char conttype[256]="\0";
	UINT32	len=255;
	pSess sdp=NULL;
	
	ccContGetType(&content,conttype,&len);
	
	if(!strcmp("application/sdp",conttype)){
		//message body is sdp
		if(RC_OK==ccContGetBody(&content,sessbuf,sesslen)){
			;//SessNewFromStr(&sdp,bodybuf);
		}else{
			printf("ccContGetBody fail! without body\n");
			*sesslen =0;
		}
	}else{
		*sesslen =0;
	}
}

int QuPreAnswerCall(pCCCall incomecall ,SipReq preq ,pSipDlg callleg)
{
	int id;
	char phonenumber[64]="0";	
	pSess sess=NULL;
	pCont msgbody=NULL;
	
	char *displayname;
	int callmode=0;
	id = QuGetFreeCall();
	printf("free qcall id = %d\n",id);
	printf("IN:QuPreAnswerCall()\n");
	
	displayname = GetReqFromDisplayname(preq);
	if (!displayname){
		displayname=strdup("");
	}
	getPhoneFromReq(preq,phonenumber);
			
	//no free call ,reject incomecall
	if (-1==id){
		printf("!!!!!\nno free call ,reject incomecall\n");
		ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
		ccCallFree(&incomecall);
		return -1;
	}
    
    //check DND
	if (qt_state.dnd==1){
		printf("!!!!!\nQT State is DND ,reject incomecall\n");
		ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
		ccCallFree(&incomecall);
		return -1;
	}
    
    //check if the sip is registered.
	/*
	if (!reg.startkplive){
		printf("!!!!!\nhave not register, regect incomingcall\n");
		ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
		ccCallFree(&incomecall);
		return -1;
	}
    */
	char *callid = NULL;
	callid=(char*)sipReqGetHdr(preq,Call_ID);
	qcall[id].callid=strdup(callid);
	
	if (qcall[1-id].callid!=NULL && qcall[id].callid!=NULL){
		if (!strcmp(qcall[1-id].callid,qcall[id].callid)){
			printf("%s,%s\n",qcall[1-id].callid,qcall[id].callid);
			return -1;	
		}
	}


	if (sipReqGetHdr(preq,P_Asserted_Identity)!=NULL){
		SipIdentity *p=(SipIdentity*)sipReqGetHdr(preq,P_Asserted_Identity);

		if (p->address->display_name!=NULL){
			//sscanf(p->address->display_name,"\"%[^\"]",p_display_name);
			printf("P-Asserted-Identity:display_name=%s\n",p->address->display_name);
		}

		if (p->address->addr!=NULL){
			//sscanf(p->address->addr,"%*[^:]:%[^@]",p_phone);
			printf("P-Asserted-Identity:display_name=%s\n",p->address->addr);
			strcpy(qcall[id].display_name ,p->address->addr);
		}else{
			strcpy(qcall[id].display_name,"");
		}

	}

	//2011-05-10 move to here from the place before Busy Forwarding
	ccContNewFromReq(&msgbody,preq);
	UINT32 contlen=0;
	ccContGetLen(&msgbody,&contlen);
	char *contbuf;
	
	if (0 < contlen){

		contbuf = (char*) malloc (contlen+8);
		memset(contbuf,0, contlen+8);

		QuContToSessToStr(msgbody,contbuf,&contlen);
		ccContFree(&msgbody);

		if (NULL!=qcall[id].rsdp){
			SessFree(&(qcall[id].rsdp));
		}
		
		SessNewFromStr(&qcall[id].rsdp,contbuf);
		free(contbuf);

		callmode = check_incoming_call_mode(qcall[id].rsdp);
		printf("Incoming call with SDP , callmode=%d\n",callmode); 
		
	}else{
		free(contbuf);
		/*2010-0629 invite without SDP*/
		qcall[id].rsdp=NULL;
		callmode=VIDEO;
		printf("Incoming call without SDP\n");
	}
	
	/* identy is Quanta product*/
	if (qcall[id].rsdp != NULL){
		QuSetIsQuantaProduct(id , qcall[id].rsdp);
	}

	if(callmode == -1) {
	    
	    printf("!!!!All codecs are not supported. Reject by 488.\n");
	    
	    pMediaPayload payload=NULL;
	    pSessMedia audio_media=NULL;
	    pSessMedia video_media=NULL;
	    pSess sess=NULL;
	    char sessbuf[SESS_BUFF_LEN]="0";
	    INT32 sesslen = 4096;
	    pCont	msgbody=NULL;
	    
	    GenAudioMedia(&audio_media);
	    GenVideoMedia(&video_media);
	    SessNew(&sess,profile.ip,audio_media,media_bandwidth);
	    SessAddMedia(sess,video_media);

	    if (SessToStr(sess,sessbuf,&sesslen)==RC_OK) 
		SessFree(&sess);
	    
	    ccContNew(&msgbody,"application/sdp",sessbuf);
	    
	    SipRsp rsp=sipRspNew();
	    if(ccNewRspFromCall(&incomecall,&rsp,488,"Not Acceptable Here",msgbody,GetReqViaBranch(preq),callleg)==RC_OK){
		char warning_text[256];
		snprintf(warning_text,256*sizeof(char),"305 %s \"Incompatible Media Format\"",profile.proxy_server);
		printf("warning text:%s\n",warning_text);
		ccRspAddHeader(rsp,Warning,warning_text);
		
		ccSendRsp(&incomecall,rsp);
	    }	
	    
	    ccContFree(&msgbody);
	    //ccRejectCallEx(&incomecall,488,"Not Acceptable Here",NULL,rsp);
	    sipRspFree(rsp);
	    ccCallFree(&incomecall);
	    if (qcall[id].rsdp!=NULL){
		SessFree(&(qcall[id].rsdp));
	    }
	    return -1;
	}
	
	bool checking_disable_condition = false;
	bool is_ims_conference = false;
	
	SipContact *pContact;
	pContact=(SipContact*)sipReqGetHdr(preq,Contact);
	
	if(pContact && pContact->sipContactList)
	{
	    int i=0;
	    SipParam* parameter = NULL;
	    if(pContact->sipContactList->ext_attrs) {
		parameter = pContact->sipContactList->ext_attrs;
	    }
	    for(i=0;i<pContact->sipContactList->numext_attrs;i++) {
		if(parameter->name) 
		{
		    if(!strncasecmp(parameter->name,"isfocus",7))
		    {
			printf("is_ims_conference = true\n");
			is_ims_conference=true;
			qcall[id].isquanta = QUANTA_PTT;
		    }
		}
		if(parameter->next) parameter = parameter->next;
	    }
	}
	
	/*
	if (qcall[id].isquanta == QUANTA_PTT && (QuCheckInSession())){
		SipReferredBy *referred_by_msg=NULL;
		referred_by_msg = (SipReferredBy*)sipReqGetHdr(preq,Referred_By);
		char referby_phone[64];
		if (referred_by_msg) {
			SipURL tourl=sipURLNewFromTxt(referred_by_msg->address->addr);
			strcpy(referby_phone,sipURLGetUser(tourl));
			printf("[SIP]PTT mode:referby phone = %s\n",referby_phone);
			// check refer-by phone is the same with another phone
			printf("[SIP]PTT mode:another phone = %s\n",qcall[1-id].phone);
			if (strcmp(qcall[1-id].phone , referby_phone)){
				printf("!!!!!\n[SIP]PTT mode, phone and referby phone are different,reject incomecall \n");
				  ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
				  g_inviteProgress=FALSE;
				  ccCallFree(&incomecall);
				  if (qcall[id].rsdp!=NULL){
					  SessFree(&(qcall[id].rsdp));
				  }
				  return -1;
			}
		}
	}
	 */

	if((qcall[id].isquanta == QUANTA_PTT)){
	  checking_disable_condition = true;

	  if (sipReqGetHdr(preq,Remote_Party_ID)!=NULL){
	  		SipRemotePartyID *p=(SipRemotePartyID*)sipReqGetHdr(preq,Remote_Party_ID);

	  		if (p->addr->display_name!=NULL){
	  			//sscanf(p->address->display_name,"\"%[^\"]",p_display_name);
	  			printf("P-Asserted-Identity:display_name=%s\n",p->addr->display_name);
	  		}

	  		if (p->addr->addr!=NULL){
	  			//sscanf(p->address->addr,"%*[^:]:%[^@]",p_phone);
	  			printf("P-Asserted-Identity:display_name=%s\n",p->addr->addr);
	  			strcpy(qcall[id].display_name ,p->addr->addr);
	  		}else{
	  			strcpy(qcall[id].display_name,"");
	  		}

	  	}

	}
	
	printf("checking_disable_condition: %d\n",checking_disable_condition);
	
	//2011-05-17 move to here for n-way v-conf,  change the if()
	//processing other incoming call
	if (g_inviteProgress && !checking_disable_condition){
		printf("!!!!!!\nOther incoming call is processing,reject this call now\n");
		ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
		ccCallFree(&incomecall);
		if (qcall[id].rsdp!=NULL){
			SessFree(&(qcall[id].rsdp));
		}
		return -1;
	}
	g_inviteProgress=TRUE;
	
	//If the call is a transfer call or a conference has started, avoid checking
	if(!checking_disable_condition) {
		int i;	
		 
		for(i=0;i<NUMOFCALL;i++){
		  if (qcall[i].call_obj!=NULL)
		  {
			  printf("!!!!!\nAlready had voice/video call id=%d and call waiting is disable ,reject incomecall \n",i);
			  ccRejectCall(&incomecall,486,"BUSY LINE",NULL);
			  g_inviteProgress=FALSE;
			  ccCallFree(&incomecall);
			  if (qcall[id].rsdp!=NULL){
				  SessFree(&(qcall[id].rsdp));
			  }
			  return -1;
		  }
		}
	  
	}
	
	//ccCallNew(&qcall[id].call_obj);
	qcall[id].call_obj = incomecall;
	qcall[id].callstate=EMPTY;
	
	strcpy(qcall[id].phone, phonenumber);
	
	ccCallSetUserInfo(&qcall[id].call_obj,reg.user);
	
	qcall[id].media_type = callmode;

	if(is_ims_conference || qcall[id].isquanta == QUANTA_PTT) {
		printf("is_ims_conference=%d,qcall[id].isquanta=%d\n",is_ims_conference,qcall[id].isquanta);
		printf("[SIP] Receive an PTT call from PTT\n");
	}

	printf("fw_incoming_call:displayname=%s,phonenumber=%s ,callmode=%d\n",displayname,phonenumber,callmode);
	
	SipRsp rsp=sipRspNew();
	if(ccNewRspFromCall(&incomecall,&rsp,180,"Ringing",NULL,GetReqViaBranch(preq),callleg)==RC_OK){
		//printf("\n%s\n",sipRspPrint(rsp));
		if (g_prack_enable){
			ccRspAddHeader(rsp,Require,"100rel");
		}
		ccRspAddHeader(rsp,Accept_Language,"en");
		//ccRspAddHeader(rsp,Allow_Events,"talk,hold,conference");
		//ccRspAddHeader(rsp,User_Agent,g_user_agent);
		ccRspAddHeader(rsp,Allow_Events,"talk,hold");
		ccSendRsp(&incomecall,rsp);
		sipRspFree(rsp);
	}
	
	return id;
}

int check_incoming_call_mode (pSess sess){
	int i,j;
	int media_size;
	UINT32 rtpmap_size;
		
	pSessMedia media=NULL;
	pMediaPayload payload=NULL;
	MediaType media_type;
	
	char payloadbuf[256]="0";
	int payloadlen=255;
	
	char payloadtype[32]="0";
	int payloadtypelen;
	
	bool is_video_match=false;
	bool is_audio_match=false;
	
	
	/*set call mode ;0:voice call 1:video call*/
	SessGetMediaSize(sess,&media_size);
	for (i=0;i<media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		if (media_type==MEDIA_VIDEO){
			/*chect if video codec has H264*/
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					payloadlen=255;
					MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
					char payloadtmp[128]="0";
					strcpy (payloadtmp,payloadbuf);
					char *pch = strtok (payloadtmp,"/");
					if (!strcasecmp(pch,"H264")){
						//return VIDEO;
						is_video_match=true;
						break;
					}
				}
			}
		}else if (media_type==MEDIA_AUDIO){
			for(j=0;j<rtpmap_size;j++){
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					payloadlen=255;
					payloadtypelen=31;
	
					MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
					MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
					//char *slash=strchr(payloadbuf,'/');
					char payloadtmp[40]="0";
					strcpy (payloadtmp,payloadbuf);
					//printf("%s\n",payloadtmp);
					char *pch = strtok (payloadtmp,"/");
					int audio_type = atoi(payloadtype);
					
					if (audio_type==0 || audio_type==8 || audio_type==9 || !strcasecmp(pch,G7221_NAME)){
						
						is_audio_match=true;
						break;
					}
				}	
			}
		}
	}
	
	if (is_video_match){
		return VIDEO;
	}else if (is_audio_match){
		return AUDIO;
	}else{
		return -1;
	}
}

int QuOfferToAnswerToStr(int call_id, pSess offer, char *sessbuf, int *sesslen){
	pSess sess=NULL;
	int media_size;
	pSessMedia answer_media=NULL;
	//pSessMedia video_media=NULL;
	pSessMedia media=NULL;
	pMediaPayload payload=NULL;
	pMediaPayload fmtp_payload=NULL;
	UINT32 rtpmap_size;
	int i,j;
	MediaType media_type;

	char payloadbuf[256]="0";
	int payloadlen=255;
	
	char payloadtype[32]="0";
	int payloadtypelen;
	
	bool is_video_match=false;
	bool is_video_add_session=false;
	bool is_audio_match=false;
	bool is_audio_add_session=false;
	
	char audio_priority[NUMOFAUDIOCODEC][16];
	
	memset(audio_priority[0],0,sizeof(audio_priority[0]));
	strcpy(audio_priority[0],sip_config.qtalk.sip.audio_codec_preference_1);
	//strcpy(audio_priority[0],OPUS_NAME);

	memset(audio_priority[1],0,sizeof(audio_priority[1]));
	strcpy(audio_priority[1],sip_config.qtalk.sip.audio_codec_preference_2);
	//strcpy(audio_priority[1],PCMU_NAME);

	memset(audio_priority[2],0,sizeof(audio_priority[2]));
	strcpy(audio_priority[2],sip_config.qtalk.sip.audio_codec_preference_3);
	//strcpy(audio_priority[2],PCMA_NAME);

	/*
	memset(audio_priority[3],0,sizeof(audio_priority[3]));
	strcpy(audio_priority[3],G722_NAME);
	*/
	int priority[NUMOFAUDIOCODEC];
	
	//printf("Offer-Answer Audio codec 1=%s,Audio codec 2=%s\n",audio_priority[0],audio_priority[1]);
	
	SessGetMediaSize(offer,&media_size);
	printf("SessGetMediaSize=%d\n",media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(offer,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		printf("rtpmap_size=%d\n",rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			int k;
			
			for(j=0;j<rtpmap_size;j++){
				if (!is_audio_match){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
	
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
						//char *slash=strchr(payloadbuf,'/');
						char payloadtmp[40]="0";
						strcpy (payloadtmp,payloadbuf);
						printf("%s\n",payloadtmp);
						char *pch = strtok (payloadtmp,"/");
						int audio_type = atoi(payloadtype);
						for(k=0;k<NUMOFAUDIOCODEC;k++){
							if (!strcasecmp(PCMU_NAME,audio_priority[k]) && 0==audio_type ){
								printf("set PCMU ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(PCMA_NAME,audio_priority[k]) && 8==audio_type ){
								printf("set PCMA ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(G722_NAME,audio_priority[k]) && 9==audio_type ){
								printf("set G722 ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(pch,audio_priority[k])){
									
								pch = strtok(NULL,"");
									
								if (!strcasecmp(G7221_NAME,audio_priority[k])){
									printf("G7221 payloadtype=%s\n",payloadtype);
									if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
										payloadlen=255;
										MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
										//char *pch = strtok (payloadtmp,"/");
										//printf("%s\n",payloadbuf);
										char fmtp_tmp[40]="0";
										strcpy (fmtp_tmp,payloadbuf);
										char *fmtp_pch = strtok (fmtp_tmp,"=");
										//printf("%s\n",fmtp_pch);
										fmtp_pch = strtok(NULL,"");
										//printf("%s\n",fmtp_pch);
										/*
										printf("pch=%s\n",pch);
										pch = strtok(NULL,"");
										printf("pch=%s\n",pch);
										*/
										if (!strcmp(audio_codec[G7221].sample_rate,pch) && !strcmp(audio_codec[G7221].fmap,fmtp_pch)){
											printf("sample rate=%s,bitrate=%s\n",pch,fmtp_pch);
											printf("set %s ok!\n",audio_priority[k]);
											is_audio_match=true;
											break;
										}
											
									}
								}else if (!strcasecmp(OPUS_NAME,audio_priority[k])){
									printf("set OPUS ok!\n");
									is_audio_match=true;
									break;
								}
								
							}

						}	
							
					}
						
				}
			}
			if (!is_audio_match){
				printf("!!!!!\nNo match audio_codec ok!\n");
			}
		}else if (media_type==MEDIA_VIDEO){
			//bool is_video_match=false;
			
			bool force_select_first_priority = false;
			int h264_rtpmap_size = 0;
			for(j=0;j<rtpmap_size;j++){
					
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
						
						char *pch = strtok (payloadtmp,"/");
						if (!strcasecmp(pch,"H264")){
							h264_rtpmap_size++;
						}
					}
			}
			
			if(!is_video_match){
				for(j=0;j<rtpmap_size;j++){
					printf("rtpmap_size=%d, j=%d\n",rtpmap_size,j);
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
				  
						payloadtypelen=31;
						MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
						payloadlen=255;
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
						
						char *pch = strtok (payloadtmp,"/");
						printf("%s\n",payloadbuf);
						
						if (!strcasecmp(pch,"H264")){
							
							//rtpmap_size =1 , do not check packetization mode
							if (h264_rtpmap_size==1 || force_select_first_priority){
								printf("Set VideoCodec H264 ok!\n");
								is_video_match=true;
								break;
							}else{
								
								MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
								pMediaPayload fmap_payload=NULL;
								SessMediaGetRTPParam(media,FMTP,payloadtype,&fmap_payload);
								
								QuH264Parameter h264_parameter;
								memset(&h264_parameter,0,sizeof(QuH264Parameter));
								QuParseH264Parameter(fmap_payload,&h264_parameter);
								if (h264_parameter.packetization_mode == 0){
									printf("Check pkt_mode,Set VideoCodec H264 ok!\n");
									is_video_match=true;
									break;
								}
								
								//select the last video media , but no one set pkt-mode , need select first H264 payload 
								printf("j=%d,rtpmap_size=%d\n",j,rtpmap_size);
								if (j == rtpmap_size-1){
									printf("force_select_first_priority\n");
									force_select_first_priority=true;
									j=-1;
								}
							}
						}
						//payload = NULL;
					}
				}
			}
			if (!is_video_match){
				printf("!!!!!\nNo match video_codec ok!\n");
				break;
			}
		}else if (media_type==MEDIA_APPLICATION){
			printf("media_type==MEDIA_APPLICATION\n");
			payload=NULL;
		}else if (media_type==MEDIA_IMAGE){
			printf("media_type==MEDIA_IMAGE\n");
			payload=NULL;
		}else if (media_type==MEDIA_MESSAGE){
			printf("media_type==MEDIA_MESSAGE\n");
			payload=NULL;
		}else{
			printf("media_type no match\n");
			payload=NULL;
		}

		if (NULL!=payload){

			int port;
			int ptime;
			if (media_type==MEDIA_AUDIO){
				//port = LAUDIOPORT;
				port = qcall[call_id].qt_call->mCallData.local_audio_port;
				ptime=20;
			}else if (media_type==MEDIA_VIDEO && !is_video_add_session){
				UINT32 rvideo_port;
				SessMediaGetPort(media,&rvideo_port);
				if (0==rvideo_port){//remote video port=0, most response local video port 0
					port=0;
				}else{
					//port = LVIDEOPORT;
					port = qcall[call_id].qt_call->mCallData.local_video_port;
				}
				ptime=20;				
			}
			//SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,payload);
			pMediaPayload matchpaylod=NULL;
			
			MediaPayloadDup(payload,&matchpaylod);
			SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,matchpaylod);
			
			/*********FMAP Test*******/
			/*2010-05-19*/
			if(RC_OK==SessMediaGetRTPParam(media,FMTP,payloadtype,&payload)){
				pMediaPayload matchfmap=NULL;
				/*2011-03-14*/
				if(media_type==MEDIA_VIDEO && !is_video_add_session) {
				  QuVideoFmtpAnswer(call_id,payload,payloadtype,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);
				  
					/*2011-06-24 add bandwidth value*/
					//SessMediaAddBandwidth(answer_media,"TIAS",H264_TIAS_BANDWIDTH);
					int tias_bandwidth = media_bandwidth * 1000;
					SessMediaAddBandwidth(answer_media,"TIAS",tias_bandwidth);

				}
				else if(media_type==MEDIA_AUDIO){
				  MediaPayloadDup(payload,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);
				}
			}
			/***************/
			
			/*********rtcp-fb*******/

			if(media_type==MEDIA_VIDEO) { 
				if (QuSessMediaGetAttribute(media,"rtcp-fb","pli")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* nack pli");
				}
				if(QuSessMediaGetAttribute(media,"rtcp-fb","fir")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* ccm fir");//a=rtcp-fb:109 ccm fir
				}
			}
			/***************/
			if(media_type==MEDIA_AUDIO) {
				if(QuSetIsQuantaProduct(-1, offer)){
					SessMediaAddAttribute(answer_media,"tool:",g_product);//a=tool:qsip v.0.0.1
				}

#if AUDIO_FEC
				printf("[AUDIO_FEC]payloadtype=%s\n",payloadtype);
				if(QuSetFec( payloadtype,offer )==QU_OK){
					pMediaPayload fec_payload=NULL;
					char rtpmap[32]="0";
					
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,"red/90000");
					SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                    
					memset(rtpmap,0,32*sizeof(char));
					snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
					//printf("rtpmap=%s\n",rtpmap);
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,rtpmap);
					SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                    
					//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
				}
#endif
				// check caller have srtp parameter
				char srtp_key[256];
				if (enable_srtp){
				if (QuSessMediaGetCryptoSize(media) > 0){
					snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLEE_AUDIO_SRTP_KEY);
					SessMediaAddCrypto(answer_media , srtp_key);
				}
				}

			}else if(media_type==MEDIA_VIDEO) {
				//if (!strcmp(sip_config.generic_rtp.video_fec_enable,"1")){
                if (1){
					if(QuSetFec( payloadtype,offer )==QU_OK){
						pMediaPayload fec_payload=NULL;
						char rtpmap[32]="0";
						
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,"red/90000");
						SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                        
						memset(rtpmap,0,32*sizeof(char));
						snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
						//printf("rtpmap=%s\n",rtpmap);
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,rtpmap);
						SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                        
						//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
					}
				}
				
			}

					
			/*set outband dtmf*/
			//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
			if (1){
				
				for(j=0;j<rtpmap_size;j++){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
								
						if (MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)==RC_OK){
							//printf("MediaPayloadGetParameter error\n");
						
							MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
							//char *slash=strchr(payloadbuf,'/');
							char payloadtmp[40]="0";
							strcpy (payloadtmp,payloadbuf);
							char *pch = strtok (payloadtmp,"/");
							//printf("%s\n",payloadtmp);
							int dtmf_type = atoi(payloadtype);
							//printf("%d\n",dtmf_type);
							if (!strcasecmp(pch,"telephone-event")){
								dtmf_type = atoi(payloadtype);
								//printf("!!!!!!!!\n%d\n",dtmf_type);
								pMediaPayload dtmf_payload=NULL;
								MediaPayloadNew(&dtmf_payload,payloadtype,"telephone-event/8000");
								SessMediaAddRTPParam(answer_media ,RTPMAP,dtmf_payload);
							}
						}		
					}
							
				}
				/**/
			}

			// check caller have srtp parameter
			char srtp_key[256];
			if (enable_srtp){
			if (QuSessMediaGetCryptoSize(media) > 0){
				snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLEE_VIDEO_SRTP_KEY);
				SessMediaAddCrypto(answer_media , srtp_key);
			}
			}

		}

		if (NULL==sess){
			SessNew(&sess,profile.ip,answer_media,0);//bandwidth 2048kbps
			SessAddBandwidth(sess,"CT",media_bandwidth);
			
			if (media_type==MEDIA_AUDIO){
				is_audio_add_session = true;
			}else if (media_type==MEDIA_VIDEO){
				is_video_add_session = true;
			}
		}
		else {
			if (answer_media!=NULL){
				if (media_type==MEDIA_AUDIO && !is_audio_add_session){
					SessAddMedia(sess,answer_media);
					is_audio_add_session = true;
				}else if (media_type==MEDIA_VIDEO && !is_video_add_session) {
					SessAddMedia(sess,answer_media);
					is_video_add_session = true;
				}
				//it is a test code, not release to others. don't remove it
				//open 2012/01/12 patrick say balbal
				else{
					printf("other media type or add audio/video media\n");
					pSessMedia new_media=NULL;
					SessMediaDup(&media,&new_media);
					SessMediaSetPort(new_media,0);
					SessAddMedia(sess,new_media);
				}
				answer_media=NULL;
			}else{
				printf("answer_media==NULL\n");
				pSessMedia new_media=NULL;
				SessMediaDup(&media,&new_media);
				SessMediaSetPort(new_media,0);
				SessAddMedia(sess,new_media);
			}
		}
	}
	
	if (!is_audio_match && !is_video_match){
		
		return -1;
		
	}else{
		if (SessToStr(sess,sessbuf,sesslen)==RC_OK) //output string
			printf("sess sdp:\n%s\nlen=%d\n",sessbuf,*sesslen);
	
		SessFree(&sess);
		return 1;
	}
		
}


void QuNewOfferToAnswerToStr(pSess offer,int id, char *sessbuf, int *sesslen){
	pSess sess=NULL;
	int media_size;
	pMediaPayload payload=NULL;
	UINT32 rtpmap_size;
	int i,j;
	MediaType media_type;
	pSessMedia media=NULL;
	pSessMedia answer_media=NULL;
	pMediaPayload fmtp_payload=NULL;
	
	char payloadbuf[256]="0";
	int payloadlen=255;
	
	char payloadtype[32]="0";
	int payloadtypelen;
	
	bool is_video_match=false;
	bool is_video_add_session=false;
	bool is_audio_match=false;
	bool is_audio_add_session=false;
	
	char audio_priority[NUMOFAUDIOCODEC][16];
	int priority[NUMOFAUDIOCODEC];
	
	memset(audio_priority[0],0,sizeof(audio_priority[0]));
	strcpy(audio_priority[0],sip_config.qtalk.sip.audio_codec_preference_1);
	//strcpy(audio_priority[0],OPUS_NAME);

	memset(audio_priority[1],0,sizeof(audio_priority[1]));
	strcpy(audio_priority[1],sip_config.qtalk.sip.audio_codec_preference_2);
	//strcpy(audio_priority[1],PCMU_NAME);

	memset(audio_priority[2],0,sizeof(audio_priority[2]));
	strcpy(audio_priority[2],sip_config.qtalk.sip.audio_codec_preference_3);
	//strcpy(audio_priority[2],PCMA_NAME);
	/*
	memset(audio_priority[3],0,sizeof(audio_priority[3]));
	strcpy(audio_priority[3],G722_NAME);
	
	*/

	SessGetMediaSize(offer,&media_size);
	printf("SessGetMediaSize=%d\n",media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(offer,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		printf("rtpmap_size=%d\n",rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			int k;
			for(k=0;k<NUMOFAUDIOCODEC;k++){		
				if (!is_audio_match){
					for(j=0;j<rtpmap_size;j++){
						if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
							payloadlen=255;
							payloadtypelen=31;
		
							MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
							MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
							//char *slash=strchr(payloadbuf,'/');
							char payloadtmp[40]="0";
							strcpy (payloadtmp,payloadbuf);
							printf("%s\n",payloadtmp);
							char *pch = strtok (payloadtmp,"/");
							int audio_type = atoi(payloadtype);
							
							if (!strcasecmp(PCMU_NAME,audio_priority[k]) && 0==audio_type ){
								printf("set PCMU ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(PCMA_NAME,audio_priority[k]) && 8==audio_type ){
								printf("set PCMA ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(G722_NAME,audio_priority[k]) && 9==audio_type ){
								printf("set G722 ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(pch,audio_priority[k])){
									
								pch = strtok(NULL,"");
								
								//if codec is G7221 , nedd check sample rate and bit rate
								if (!strcasecmp(G7221_NAME,audio_priority[k])){
									printf("G7221 payloadtype=%s\n",payloadtype);
									if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
										payloadlen=255;
										MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
										//char *pch = strtok (payloadtmp,"/");
										//printf("%s\n",payloadbuf);
										char fmtp_tmp[40]="0";
										strcpy (fmtp_tmp,payloadbuf);
										char *fmtp_pch = strtok (fmtp_tmp,"=");
										//printf("%s\n",fmtp_pch);
										fmtp_pch = strtok(NULL,"");
										//printf("%s\n",fmtp_pch);
										/*
										printf("pch=%s\n",pch);
										pch = strtok(NULL,"");
										printf("pch=%s\n",pch);
										*/
										if (!strcmp(audio_codec[G7221].sample_rate,pch) && !strcmp(audio_codec[G7221].fmap,fmtp_pch)){
											printf("sample rate=%s,bitrate=%s\n",pch,fmtp_pch);
											printf("set %s ok!\n",audio_priority[k]);
											is_audio_match=true;
											break;
										}
											
									}
								}else if (!strcasecmp(OPUS_NAME,audio_priority[k])){
									printf("set OPUS ok!\n");
									is_audio_match=true;
									break;
								}
								
							}
						}		
					}
						
				}
			}
			if (!is_audio_match){
				printf("!!!!!\nNo match audio_codec ok!\n");
			}
		}else if (media_type==MEDIA_VIDEO){
			//bool is_video_match=false;
			bool force_select_first_priority = false;
			int h264_rtpmap_size = 0;
			for(j=0;j<rtpmap_size;j++){
					
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
						
						char *pch = strtok (payloadtmp,"/");
						if (!strcasecmp(pch,"H264")){
							h264_rtpmap_size++;
						}
					}
			}
			
			if(!is_video_match){
				for(j=0;j<rtpmap_size;j++){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
						char *pch = strtok (payloadtmp,"/");
						printf("%s\n",payloadbuf);
						if (!strcasecmp(pch,"H264")){
							//rtpmap_size =1 , do not check packetization mode
							if (h264_rtpmap_size==1 || force_select_first_priority){
								printf("Set VideoCodec H264 ok!\n");
								is_video_match=true;
								break;
							}else{
								
								MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
								pMediaPayload fmap_payload=NULL;
								SessMediaGetRTPParam(media,FMTP,payloadtype,&fmap_payload);
								
								QuH264Parameter h264_parameter;
								memset(&h264_parameter,0,sizeof(QuH264Parameter));
								QuParseH264Parameter(fmap_payload,&h264_parameter);
								if (h264_parameter.packetization_mode == 0){
									printf("Check pkt_mode,Set VideoCodec H264 ok!\n");
									is_video_match=true;
									break;
								}
								
								//select the last video media , but no one set pkt-mode , need select first H264 payload 
								printf("j=%d,rtpmap_size=%d\n",j,rtpmap_size);
								if (j == rtpmap_size-1){
									printf("force_select_first_priority\n");
									force_select_first_priority=true;
									j=-1;
								}
							}
						}
						//payload = NULL;
					}
				}
			}
			if (!is_video_match){
				printf("!!!!!\nNo match video_codec ok!\n");
				break;
			}
		}else{
			printf("media_type no match\n");
			payload=NULL;
		}
		
		if (NULL!=payload){
			int port;
			int ptime;
			if (media_type==MEDIA_AUDIO){
				//port = LAUDIOPORT;
				port = qcall[id].audio_port;
				ptime=20;
			}else if (media_type==MEDIA_VIDEO){
				UINT32 rvideo_port;
				SessMediaGetPort(media,&rvideo_port);
				if (0==rvideo_port){//remote video port=0, local most response local video port 0
					port=0;
				}else{
					//port = LVIDEOPORT;
					port = qcall[id].video_port;
				}
				ptime=20;				
			}
			//SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,payload);
			pMediaPayload matchpaylod=NULL;
			MediaPayloadDup(payload,&matchpaylod);
			payloadlen=255;
			payloadtypelen=31;
	
			MediaPayloadGetParameter(matchpaylod,payloadbuf,&payloadlen);
			MediaPayloadGetType(matchpaylod,payloadtype,&payloadtypelen);
			
			SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,matchpaylod);
		
			/*********FMAP Test*******/
			if(RC_OK==SessMediaGetRTPParam(media,FMTP,payloadtype,&payload)){
				pMediaPayload matchfmap=NULL;
				/*2011-03-14*/
				if(media_type==MEDIA_VIDEO && !is_video_add_session ) {
				  QuVideoFmtpAnswer(id,payload,payloadtype,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);

					/*add bandwidth value*/
					int tias_bandwidth = media_bandwidth * 1000;
					SessMediaAddBandwidth(answer_media,"TIAS",tias_bandwidth);
				}
				else if(media_type==MEDIA_AUDIO){
				  MediaPayloadDup(payload,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);
				}
			}
			/***************/
			
			if(media_type==MEDIA_VIDEO) { 
				if (QuSessMediaGetAttribute(media,"rtcp-fb","pli")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* nack pli");
				}
				if(QuSessMediaGetAttribute(media,"rtcp-fb","fir")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* ccm fir");//a=rtcp-fb:109 ccm fir
				}
			}
			
			if(media_type==MEDIA_AUDIO) {
				if(QuSetIsQuantaProduct(-1, offer)){
					SessMediaAddAttribute(answer_media,"tool:",g_product);//a=tool:qsip v.0.0.1
				}
#if AUDIO_FEC
				printf("[AUDIO_FEC]payloadtype=%s\n",payloadtype);
				if(QuSetFec( payloadtype,offer )==QU_OK){
					pMediaPayload fec_payload=NULL;
					char rtpmap[32]="0";
					
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,"red/90000");
					SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                    
					memset(rtpmap,0,32*sizeof(char));
					snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
					//printf("rtpmap=%s\n",rtpmap);
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,rtpmap);
					SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                    
					//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
				}
#endif
				// check caller have srtp parameter
				char srtp_key[256];
				if (enable_srtp){
				if (QuSessMediaGetCryptoSize(media) > 0){
					snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLEE_AUDIO_SRTP_KEY);
					SessMediaAddCrypto(answer_media , srtp_key);
				}
				}
			}else if(media_type==MEDIA_VIDEO) {
				//if (!strcmp(sip_config.generic_rtp.video_fec_enable,"1")){
                if (1){
					if(QuSetFec( payloadtype,offer )==QU_OK){
						pMediaPayload fec_payload=NULL;
						char rtpmap[32]="0";
						
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,"red/90000");
						SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                        
						memset(rtpmap,0,32*sizeof(char));
						snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
						//printf("rtpmap=%s\n",rtpmap);
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,rtpmap);
						SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                        
						//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
					}
				}
                // check caller have srtp parameter
                char srtp_key[256];
                if (enable_srtp){
				if (QuSessMediaGetCryptoSize(media) > 0){
					snprintf(srtp_key,256*sizeof(char),"%s inline:%s|2^20|1:32","AES_CM_128_HMAC_SHA1_80",CALLEE_VIDEO_SRTP_KEY);
					SessMediaAddCrypto(answer_media , srtp_key);
				}
                }
				
			}
			
			/*set outband dtmf*/
			//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
			if (1){
				for(j=0;j<rtpmap_size;j++){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
								
						if (MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)==RC_OK){
							//printf("MediaPayloadGetParameter error\n");
						
							MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
							//char *slash=strchr(payloadbuf,'/');
							char payloadtmp[40]="0";
							strcpy (payloadtmp,payloadbuf);
							char *pch = strtok (payloadtmp,"/");
							//printf("%s\n",payloadtmp);
							int dtmf_type = atoi(payloadtype);
							//printf("%d\n",dtmf_type);
							if (!strcasecmp(pch,"telephone-event")){
								dtmf_type = atoi(payloadtype);
								//printf("!!!!!!!!\n%d\n",dtmf_type);
								pMediaPayload dtmf_payload=NULL;
								MediaPayloadNew(&dtmf_payload,payloadtype,"telephone-event/8000");
								SessMediaAddRTPParam(answer_media ,RTPMAP,dtmf_payload);
							}
						}		
					}
				}
			}
			
		}
		
		if (NULL==sess){

			SessNew(&sess,profile.ip,answer_media,0);//bandwidth 2048kbps
			SessAddBandwidth(sess,"CT",media_bandwidth);
			
			if (media_type==MEDIA_AUDIO){
				is_audio_add_session = true;
			}else if (media_type==MEDIA_VIDEO){
				is_video_add_session = true;
			}

		}
		else {
			if (answer_media!=NULL){
				if (media_type==MEDIA_AUDIO && !is_audio_add_session){
					SessAddMedia(sess,answer_media);
					is_audio_add_session = true;
				}else if (media_type==MEDIA_VIDEO && !is_video_add_session) {
					SessAddMedia(sess,answer_media);
					is_video_add_session = true;
				}//open 2012/01/12 patrick say balbal
				else{
					printf("other media type or add audio/video media\n");
					pSessMedia new_media=NULL;
					SessMediaDup(&media,&new_media);
					SessMediaSetPort(new_media,0);
					SessAddMedia(sess,new_media);
				}
				answer_media=NULL;
			}else{
				printf("answer_media==NULL\n");
				pSessMedia new_media=NULL;
				SessMediaDup(&media,&new_media);
				SessMediaSetPort(new_media,0);
				SessAddMedia(sess,new_media);
			}
		}
	}
	
	if (SessToStr(sess,sessbuf,sesslen)==RC_OK) //output string
		printf("sess sdp:\n%s\nlen=%d\n",sessbuf,*sesslen);
	
	SessFree(&sess);
	
}

/*check remote if quanta product*/
/*callid >=0 :set qstream_q_product*/
/*callid <0 : just check if quanta product*/
int QuSetIsQuantaProduct(int callid, pSess sess){
	int media_size;
	pSessMedia media=NULL;
	MediaType media_type;
	int i;
	int res=0;
	char *pch;
	char dev_company[16]="0";
	char product_name[16]="0";
	
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		if (media_type==MEDIA_AUDIO){
			char pname[32]="0";
			UINT32 plen=31;
            
			if (RC_OK==SessMediaGetAttribute(media,"tool",pname,&plen)){
				pch = strtok (pname," :\t\n");
				if(pch)
					strcpy(dev_company,pch);
				
				pch = strtok (NULL," _");
				if (pch)
					strcpy(product_name,pch);

				printf("company=%s,project=%s\n",dev_company,product_name);
				
				if (!strcmp(dev_company,COMPANY) ){
					if (!strcmp(product_name,PROJECT_ET1)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = ET1;
						}
						
					}else if (!strcmp(product_name,PROJECT_QT1A)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QT1A;
						}
					}else if (!strcmp(product_name,PROJECT_QT1W)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QT1W;
						}
					}else if (!strcmp(product_name,PROJECT_QT1I)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QT1I;
						}
					}else if (!strcmp(product_name,PROJECT_PX1)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = ET1;
						}
					}else if (!strcmp(product_name,PROJECT_PX2)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QUANTA_PX2;
						}
					}else if (!strcmp(product_name,PROJECT_MCU)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QUANTA_MCU;
						}
					}else if (!strcmp(product_name,PROJECT_PTT)){
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QUANTA_PTT;
						}
					}else{
						if (callid>=0){
							printf("qstream_q_product(%d,TRUE)\n",callid);
							qcall[callid].isquanta = QUANTA_OTHERS;
						}
					}
					return 1;
				}else{
					if (callid>=0){
						printf("qstream_q_product(%d,FALSE)\n",callid);
						qcall[callid].isquanta = NONE_QUANTA;
					}
					return 0;
				}
			}else{
				//printf("identity_quanta=%s\n",identity_quanta);
				
				if (callid>=0){
					printf("qstream_q_product(%d,FALSE)\n",callid);
					qcall[callid].isquanta = NONE_QUANTA;
				}
				return 0;
			
			}
		}
	}
	return -1;
}

int QuVideoFmtpAnswer(int call_id, pMediaPayload remote_payload,const char *payloadtype,pMediaPayload *local_payload) {
  
  QuH264Parameter h264_parameter;
  
  if(1==QuParseH264LevelID(remote_payload,&h264_parameter)) {
    //QuPrintProfileLevelID(&profile_level_id);
  }
  else { //default = 42800A
    h264_parameter.profile_level_id.profile_idc=66; 
    h264_parameter.profile_level_id.constraint_set0_flag=true;
    h264_parameter.profile_level_id.level_idc=10;  
  }
    
  int packetization_mode;
  char answer_levle_id[8];
  memset(answer_levle_id,0,8*sizeof(char));
  
  snprintf(answer_levle_id,8*sizeof(char),"%02X",local_video_mdp.level_idc);

  char answer_fmtp[256];
  memset(answer_fmtp,0,256*sizeof(char));

  snprintf(answer_fmtp,256*sizeof(char),"profile-level-id=42E0%s;packetization-mode=%s",
		  answer_levle_id,H264PACKETMODE);

  if (local_video_mdp.max_mbps > 0){
	  snprintf(answer_fmtp,256*sizeof(char),"%s;max-mbps=%d",answer_fmtp,local_video_mdp.max_mbps);
  }
  if (local_video_mdp.max_fs > 0){
	  snprintf(answer_fmtp,256*sizeof(char),"%s;max-fs=%d",answer_fmtp,local_video_mdp.max_fs);
  }

  MediaPayloadNew(local_payload,payloadtype,answer_fmtp);
  
  return 1;
}

int QuParseH264LevelID(pMediaPayload payload,QuH264Parameter *h264_parameter) {
  
  if(payload==NULL || h264_parameter==NULL)  {
    printf("<sip> QuParseProfileLevelID wrong input\n");
    return -1;
  }
  
  int i,payloadlen=255;
  char payloadbuf[256]="0";

  payloadlen=255;
  if(RC_OK!=MediaPayloadGetParameter(payload,payloadbuf,&payloadlen))  {
    printf("<sip> QuParseProfileLevelID input overflow\n");
    return -1;
  }

  char payloadtmp[256]="0";
  strcpy (payloadtmp,payloadbuf);
  char *pch = strtok(payloadtmp,";= ");
  while(pch != NULL)
  {
    if(!strcmp(pch,"profile-level-id")) {
      pch=strtok(NULL,";= ");
      char hextmp[3];
      unsigned int profiletmp;
      hextmp[0]=*pch; hextmp[1]=*(pch+1); hextmp[2]='\0';
      
      sscanf(hextmp,"%X",&h264_parameter->profile_level_id.profile_idc);
      hextmp[0]=*(pch+2); hextmp[1]=*(pch+3); hextmp[2]='\0';
      
      sscanf(hextmp,"%X",&profiletmp);
      h264_parameter->profile_level_id.constraint_set0_flag = (bool) (profiletmp & 0x80);
      h264_parameter->profile_level_id.constraint_set1_flag = (bool) (profiletmp & 0x40);
      h264_parameter->profile_level_id.constraint_set2_flag = (bool) (profiletmp & 0x20);

      hextmp[0]=*(pch+4); hextmp[1]=*(pch+5); hextmp[2]='\0';
      sscanf(hextmp,"%X",&h264_parameter->profile_level_id.level_idc);
      
      printf("<sip> parse profile-level-id success\n");
      //return 1;
    }else{
      pch=strtok(NULL,";= ");
    }     
    pch=strtok(NULL,";= ");
  }
  //printf("<sip> do not attach the profile-level-id\n");
  return 1;
}

int QuParseH264Parameter(pMediaPayload payload,QuH264Parameter *h264_parameter) {
  
  if(payload==NULL || h264_parameter==NULL)  {
    printf("<sip> QuParseProfileLevelID wrong input\n");
    return -1;
  }
  
  int i,payloadlen=255;
  char payloadbuf[256]="0";

  payloadlen=255;
  if(RC_OK!=MediaPayloadGetParameter(payload,payloadbuf,&payloadlen))  {
    printf("<sip> QuParseProfileLevelID input overflow\n");
    return -1;
  }
  char payloadtmp[256]="0";
  strcpy (payloadtmp,payloadbuf);
  char *pch = strtok(payloadtmp,";= ");
  while(pch != NULL)
  {
    
	//printf("parameter=%s\n",pch);  
	if(!strcmp(pch,"profile-level-id")) {
      pch=strtok(NULL,";= ");
      char hextmp[3];
      unsigned int profiletmp;
      hextmp[0]=*pch; hextmp[1]=*(pch+1); hextmp[2]='\0';
      
      sscanf(hextmp,"%X",&h264_parameter->profile_level_id.profile_idc);
      hextmp[0]=*(pch+2); hextmp[1]=*(pch+3); hextmp[2]='\0';
      
      sscanf(hextmp,"%X",&profiletmp);
      h264_parameter->profile_level_id.constraint_set0_flag = (bool) (profiletmp & 0x80);
      h264_parameter->profile_level_id.constraint_set1_flag = (bool) (profiletmp & 0x40);
      h264_parameter->profile_level_id.constraint_set2_flag = (bool) (profiletmp & 0x20);

      hextmp[0]=*(pch+4); hextmp[1]=*(pch+5); hextmp[2]='\0';
      sscanf(hextmp,"%X",&h264_parameter->profile_level_id.level_idc);
      
      printf("<sip> parse profile-level-id success\n");
      //return 1;
    }else if(!strcmp(pch,"max-mbps")) {
		pch=strtok(NULL,";= ");
		int max_mbps = atoi(pch);
		printf("max-mbps=%d\n",max_mbps);
		h264_parameter->max_mbps=max_mbps;
	}else if(!strcmp(pch,"max-fs")) {
		pch=strtok(NULL,";= ");
		int max_fs = atoi(pch);
		printf("max-fs=%d\n",max_fs);
		h264_parameter->max_fs=max_fs;
	}else if(!strcmp(pch,"packetization-mode")) {
		pch=strtok(NULL,";= ");
		int packetization_mode = atoi(pch);
		printf("packetization-mode=%d\n",packetization_mode);
		h264_parameter->packetization_mode=packetization_mode;
	}else{
      pch=strtok(NULL,";= ");
    }     
    pch=strtok(NULL,";= ");
  }
  //printf("<sip> do not attach the profile-level-id\n");
  return 1;
}

void MediaCodecInfoInit(MediaCodecInfo *media_info){
	
	if (media_info!=NULL){
		memset(media_info, 0, sizeof(MediaCodecInfo));
		media_info->audio_payload_type = -1;
		media_info->video_payload_type = -1;
		media_info->dtmf_type = -1;
		media_info->media_bandwidth = -1;
		media_info->h264_parameter.max_mbps = -1;
		media_info->h264_parameter.max_fs = -1;
		media_info->RS_bps = -1;
		media_info->RR_bps = -1;
		media_info->rtcp_fb.nack.pli = -1;
		media_info->rtcp_fb.ccm.fir = -1;
	}
	
}

int QuHoldCall(char *addr)
{	
	char sessbuf[SESS_BUFF_LEN]="0";
	INT32 sess_len ;
	pCont	msgbody=NULL;
	HoldStyle hs;
	
	int media_size;
	pSessMedia sess_media=NULL;
	pMediaPayload payload=NULL;
	int i;
	
	int id;
	id = QuGetCall(addr);
	printf("call id = %d\n",id);
	if (-1==id){
		return 0 ;
	}
	
	/*2010=0705*/
	//clear old media
	SessGetMediaSize(qcall[id].lsdp,&media_size);
	printf("SessGetOldSdpMediaSize=%d\n",media_size);
	for (i=0;i<media_size;i++){
		SessRemoveMedia(qcall[id].lsdp,0);
	}
	MediaPayloadNew(&payload,audio_codec[qcall[id].audio_codec].payload,audio_codec[qcall[id].audio_codec].codec);
	SessMediaNew(&sess_media,MEDIA_AUDIO,NULL,qcall[id].audio_port,"RTP/AVP",20,RTPMAP,payload);
	
	if (G7221 == qcall[id].audio_codec){
		//MediaPayloadNew(&payload,"121","bitrate=24000");
		
		char g7221_bitrate[32]="0";
		snprintf(g7221_bitrate,32*sizeof(char),"bitrate=%s",audio_codec[G7221].fmap);
		MediaPayloadNew(&payload,audio_codec[G7221].payload,g7221_bitrate);
		SessMediaAddRTPParam(sess_media ,FMTP,payload);
	}
	
	SessMediaAddAttribute(sess_media,"tool:",g_product);//a=tool:qsip v.0.0.1

	if(1)
	//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method))
	{
		MediaPayloadNew(&payload,"101","telephone-event/8000");
		SessMediaAddRTPParam(sess_media ,RTPMAP,payload);
	}
	
	SessAddMedia(qcall[id].lsdp,sess_media);
	/**/
	
	/*set hold sdp*/
	if (SessSetHold(qcall[id].lsdp,CZEROS)!=RC_OK){
		printf("sess sethold CZEROS fail!\n");
	}
	/*rfc3108 2010-0820*/
	SessSetStatus(qcall[id].lsdp,INACTIVE);
		
	/*
	if (SessSetHold(qcall[id].lsdp,AUDIO_INACTIVE)!=RC_OK){
		printf("sess sethold AUDIO_INACTIVE fail!\n");
	}
	*/
	
	SessAddVersion(qcall[id].lsdp);

	sess_len=SESS_BUFF_LEN-1;
	if (SessToStr(qcall[id].lsdp,sessbuf,&sess_len)!=RC_OK) //output string
		printf("sess sdp fail\n");
	
	ccContNew(&msgbody,"application/sdp",sessbuf);
	
	ccModifyCall(&qcall[id].call_obj,msgbody);
	//ccMakeCall(&newcall,DialURL,msgbody,preq); //preq ccReqAddAuthz()
	ccContFree(&msgbody);
	
	return 1;
}

int QuMusicHoldCall(char *addr)
{	
	char sessbuf[SESS_BUFF_LEN]="0";
	INT32 sess_len ;
	pCont	msgbody=NULL;
	HoldStyle hs;
	
	int media_size;
	pSessMedia sess_media=NULL;
	pMediaPayload payload=NULL;
	int i;
	
	int id;
	id = QuGetCall(addr);
	printf("call id = %d\n",id);
	if (-1==id){
		return 0 ;
	}
	
	//clear old media
	SessGetMediaSize(qcall[id].lsdp,&media_size);
	printf("SessGetOldSdpMediaSize=%d\n",media_size);
	for (i=0;i<media_size;i++){
		SessRemoveMedia(qcall[id].lsdp,0);
	}
	MediaPayloadNew(&payload,audio_codec[qcall[id].audio_codec].payload,audio_codec[qcall[id].audio_codec].codec);
	SessMediaNew(&sess_media,MEDIA_AUDIO,NULL,qcall[id].audio_port,"RTP/AVP",20,RTPMAP,payload);
	
	if (G7221 == qcall[id].audio_codec){
		//MediaPayloadNew(&payload,"121","bitrate=24000");
		
		char g7221_bitrate[32]="0";
		snprintf(g7221_bitrate,32*sizeof(char),"bitrate=%s",audio_codec[G7221].fmap);
		MediaPayloadNew(&payload,audio_codec[G7221].payload,g7221_bitrate);
		SessMediaAddRTPParam(sess_media ,FMTP,payload);
	}
	
	SessMediaAddAttribute(sess_media,"tool:",g_product);//a=tool:qsip v.0.0.1

	if(1)
	//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method))
	{
		MediaPayloadNew(&payload,"101","telephone-event/8000");
		SessMediaAddRTPParam(sess_media ,RTPMAP,payload);
	}
	
	SessAddMedia(qcall[id].lsdp,sess_media);
	/**/
	
	/*rfc3108 2010-0820*/
	//SessSetStatus(qcall[id].lsdp,SENDONLY);
		
	/*set sendonly in media description*/
	if (SessSetHold(qcall[id].lsdp,AUDIO_SENDONLY)!=RC_OK){
		printf("sess sethold AUDIO_INACTIVE fail!\n");
	}
	
	
	SessAddVersion(qcall[id].lsdp);
	
	sess_len=SESS_BUFF_LEN-1;
	if (SessToStr(qcall[id].lsdp,sessbuf,&sess_len)!=RC_OK) //output string
		printf("sess sdp fail\n");
	
	ccContNew(&msgbody,"application/sdp",sessbuf);
	
	//ccMakeCall(&newcall,"sip:7817689654@viperdevas01.walvoip.net",msgbody,newreq);
	ccModifyCall(&qcall[id].call_obj,msgbody);
	//ccMakeCall(&newcall,DialURL,msgbody,preq); //preq ccReqAddAuthz()
	ccContFree(&msgbody);
	
	return 1;
}

int QuUnHoldCall(char *addr)
{
	char sessbuf[SESS_BUFF_LEN]="0";
	INT32 sess_len ;
	pCont	msgbody=NULL;
	HoldStyle hs;
	
	int id;
	id = QuGetCall(addr);
	printf("call id = %d\n",id);
	if (-1==id){
		return 0 ;
	}
	
	if (SessSetUnHold(qcall[id].lsdp ,profile.ip)==RC_OK){
		SessSetStatus(qcall[id].lsdp,SENDRECV);
		printf("sess UnHold is done\n");
		SessAddVersion(qcall[id].lsdp);
	}
	else printf("sess UnHold fail!\n");
	
	sess_len=SESS_BUFF_LEN-1;
	if (SessToStr(qcall[id].lsdp,sessbuf,&sess_len)!=RC_OK) //output string
		printf("sess sdp fail\n");
	
	ccContNew(&msgbody,"application/sdp",sessbuf);
	
	ccModifyCall(&qcall[id].call_obj,msgbody);
	ccContFree(&msgbody);
	
	qcall[id].request_mode=REQUEST_UNHOLD;
	
	return 1;
}

//Receive update
int QuRevUpdate(pCCCall call_obj, SipReq req, pSipDlg dlg)
{
    int res=0;
    char *displayname = GetReqFromDisplayname(req);
    if (!displayname){
        displayname=strdup("");
    }
    
    char phonenumber[64]="0";
    getPhoneFromReq(req,phonenumber);
    SipRsp rsp=NULL;
    pSess sess=NULL;
    HoldStyle hs;
    char sessbuf[SESS_BUFF_LEN]="0";
    int sesslen=SESS_BUFF_LEN-1;
    pCont content=NULL;
    pCont msgbody=NULL;
    int id = QuGetCall(phonenumber);
    printf("id=%d\n",id);
    if (-1==id){
        return 0 ;
    }
    
    /*
     Check uhhold or transfer for Cisco
    */
    if (sipReqGetHdr(req,Remote_Party_ID)!=NULL){
		SipRemotePartyID *p=(SipRemotePartyID*)sipReqGetHdr(req,Remote_Party_ID);

		char p_phone[64];
		char p_display_name[64];
		if (p->addr->display_name!=NULL){
			sscanf(p->addr->display_name,"\"%[^\"]",p_display_name);
		}else{
			strcpy(p_display_name,"");
		}

		if (p->addr->addr!=NULL){
			sscanf(p->addr->addr,"%*[^:]:%[^@]",p_phone);
		}else{
			strcpy(p_phone,"");
		}

		printf("Remote_Party_ID:phone=%s,display_name=%s,addr=%s\n",phonenumber,p_display_name,p_phone);
		if (strcmp(phonenumber,p_phone)){
			is_cisco_transfer = 1;
		}else{
			is_cisco_transfer = 0;
		}

	}

    ccContNewFromReq(&content,req);
    
    UINT32 contlen=0;
    ccContGetLen(&content,&contlen);
    char *contbuf = (char*) malloc (contlen+8);
    memset(contbuf,0, contlen+8);
    
    QuContToSessToStr(content,contbuf,&contlen);
    ccContFree(&content);
    
    //session timer
    SipSessionExpires *se=(SipSessionExpires *)sipReqGetHdr(req,Session_Expires);
    
    if (0!=contlen){
        unsigned long oldversion,newversion;
        SessNewFromStr(&sess,contbuf);
        SessGetVersion(sess,&newversion);
        SessFree(&sess);
        
        SessGetVersion(qcall[id].rsdp,&oldversion);
        printf("old version =%lu , new version =%lu\n",oldversion,newversion);
        if (newversion <= oldversion){
            //if (newversion == oldversion){
            /*session audit*/
            if (newversion==oldversion){
                sesslen=1023;
                SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
                ccContNew(&msgbody,"application/sdp",sessbuf);
                
                if(ccNewRspFromCall(&call_obj,&rsp,200,"ok",msgbody,GetReqViaBranch(req),dlg)==RC_OK){
                    ccSendRsp(&call_obj,rsp);
                    sipRspFree(rsp);
                }
                ccContFree(&msgbody);
            }
            g_inviteProgress=FALSE;
            return 0 ;
        }
        
        //memcpy(&qcall[id].rsdp ,sess , sizeof(sess));//save new sdp
        if (NULL!=qcall[id].rsdp){
            SessFree(&(qcall[id].rsdp));
        }
        SessNewFromStr(&qcall[id].rsdp,contbuf);
        free(contbuf);
        
        SessCheckHold(qcall[id].rsdp,&hs);
        if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){
            printf("It's a hold sdp\n");
            
            g_inviteProgress=TRUE;
            qcall[id].g_RequestProgress=TRUE;
            
            if(qcall[id].request_mode==REQUEST_SWITCH_TO_AUDIO || qcall[id].request_mode==REQUEST_SWITCH_TO_VIDEO) {
                printf("Hold > Switch, cancel the switch request!\n");
                
                int switch_mode;
                if (REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode){
                    switch_mode = AUDIO;
                }else if (REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode){
                    switch_mode = VIDEO;
                }
                
                qcall[id].request_mode=REQUEST_EMPTY;
            }
            
            printf("fw_inbound_hold :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
            
            //!----For QT----//
            
            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
            {
                // Enter the critical section
#if MUTEX_DEBUG
                printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                
                qcall[id].qt_call->mCallData.is_recv_hold = true;
                qcall[id].qt_call->mCallData.call_state = CALL_STATE_HOLD;
                qcall[id].qt_call->mCallData.is_reinvite = true;
                
                g_event_listner->onCallIncoming(qcall[id].qt_call);
                g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                printf("id=%d,qt=%d,state=%d\n",id,qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state);
                
#if MUTEX_DEBUG
                printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                //----------------------
            }
            QuReInviteOfferToAnswer(id,qcall[id].rsdp);
            
            if (HOLD != qcall[id].callstate && HOLD_BEHOLD != qcall[id].callstate && qcall[id].request_mode!=REQUEST_HOLD){
                qcall[id].callstate = BEHOLD;
#if 0
                if (hs==CZEROS || AUDIO_INACTIVE==hs){
                    //QStreamPara media;
                    //set_media_para(&media,qcall[id].session,-1,NULL,NULL,-1,-1,-1,-1,NULL,-1,0);
                    
                }else{
                    
                    printf("hold state,but not CZEROS or AUDIO_INACTIVE\n");
                    char remote_a_ip[32]="0";
                    char remote_v_ip[32]="0";
                    int laudio_port=0,lvideo_port=0;
                    int raudio_port=0,rvideo_port=0;
                    
                    MediaCodecInfo media_info;
                    MediaCodecInfoInit(&media_info);
                    
                    int callmode;
                    
                    QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
                    QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
                    QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
                    QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
                    QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
                    
                    laudio_port = qcall[id].audio_port;
                    lvideo_port = qcall[id].video_port;
                    
                    if(0!=rvideo_port){
                        callmode=VIDEO;
                    }else{
                        callmode=AUDIO;
                    }
                    /*
                     printf("QuStreamSwitchCallMode:session=%d,Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
                     qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
                     
                     */
                    printf("id=%d, phone=%s\n",id,qcall[id].phone);
                    QStreamPara media;
                    set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);
                }
#endif
            }else{
                printf("qcall[id].callstate = HOLD_BEHOLD\n");
                qcall[id].callstate = HOLD_BEHOLD;
            }
        }else{
            printf("QuRevUpdate:Not Hold SDP\n");
            unsigned long oldversion,newversion;
            unsigned long oldid,newid;
            
            //SessGetVersion(qcall[id].lsdp,&oldversion);
            //SessGetSessID(qcall[id].lsdp,&oldid);
            printf("callstate=%d\n",qcall[id].callstate);
            if (qcall[id].request_mode==REQUEST_HOLD || HOLD==qcall[id].callstate || (g_inviteProgress && BEHOLD!=qcall[id].callstate && HOLD_BEHOLD!=qcall[id].callstate)) {
                /*if callstate==hold , receiving swicting request will auto reject(406)*/
                if (BEHOLD!=qcall[id].callstate && HOLD_BEHOLD!=qcall[id].callstate){
                    pCont msgbody=NULL;
                    SipRsp rsp=NULL;
                    char sessbuf[SESS_BUFF_LEN]="0";
                    int sesslen=SESS_BUFF_LEN-1;
                    //SessAddVersion(qcall[id].lsdp);//add sdp version
                    SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
                    ccContNew(&msgbody,"application/sdp",sessbuf);
                    
                    if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,406,"Not Accept",NULL,GetReqViaBranch(req),dlg)==RC_OK){
                        printf("callstate==hold,auto reject switch\n");
                        printf("request_mode=%d\n",qcall[id].request_mode);
                        printf("callstate=%d\n",qcall[id].callstate);
                    }
                    ccSendRsp(&qcall[id].call_obj,rsp);
                    sipRspFree(rsp);
                    ccContFree(&msgbody);
                    return 1;
                }
            }else if (ACTIVE == qcall[id].callstate || EMPTY == qcall[id].callstate){
                /*if already send request , receiving swicting request will auto reject(406)*/
                if ((REQUEST_HOLD==qcall[id].request_mode)||(REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode)||(REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode)){
                    pCont msgbody=NULL;
                    SipRsp rsp=NULL;
                    char sessbuf[SESS_BUFF_LEN]="0";
                    int sesslen=SESS_BUFF_LEN-1;
                    //SessAddVersion(qcall[id].lsdp);//add sdp version
                    SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
                    ccContNew(&msgbody,"application/sdp",sessbuf);
                    
                    if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,406,"Not Accept",NULL,GetReqViaBranch(req),dlg)==RC_OK){
                        printf("request_mode==REQUEST_HOLD||REQUEST_SWITCH_TO_AUDIO||REQUEST_SWITCH_TO_VIDEO,auto reject switch\n");
                        printf("request_mode=%d\n",qcall[id].request_mode);
                    }
                    ccSendRsp(&qcall[id].call_obj,rsp);
                    sipRspFree(rsp);
                    ccContFree(&msgbody);
                    
                    return 1;
                }
            }
            
            
            g_inviteProgress=TRUE;
            qcall[id].g_RequestProgress=TRUE;
            /*2010-0621*/
            QuReInviteOfferToAnswer(id,qcall[id].rsdp);
            
            //SessGetVersion(qcall[id].lsdp,&newversion);
            //SessGetSessID(qcall[id].lsdp,&newid);
            //printf("old version =%lu , new version =%lu\n",oldversion,newversion);
            //printf("old id =%ld , new id =%ld\n",oldid,newid);
            
            printf("qcall[id].callstate=%d\n",qcall[id].callstate);
            
            if (BEHOLD==qcall[id].callstate){ /*2011/7/15 marked by nicky || HOLD==qcall[id].callstate*/
                
                qcall[id].callstate = ACTIVE;
                
                char remote_a_ip[32]="0";
                char remote_v_ip[32]="0";
                int laudio_port=0,lvideo_port=0;
                int raudio_port=0,rvideo_port=0;
                
                MediaCodecInfo media_info;
                MediaCodecInfoInit(&media_info);
                
                int callmode;
                
                if (qcall[id].media_type==AUDIO){
                    int i;
                    int media_size;
                    pSessMedia sess_media=NULL;
                    MediaType media_type;
                    
                    /*2010-07-13 remove part of video*/
                    SessGetMediaSize(qcall[id].lsdp,&media_size);
                }
                
                QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
                QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
                QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
                QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
                QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
                
                laudio_port = qcall[id].audio_port;
                lvideo_port = qcall[id].video_port;
                
                if(0!=rvideo_port){
                    callmode=VIDEO;
                }else{
                    callmode=AUDIO;
                }
                
                printf("fw_inbound_resume :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
                
                //if (qcall[id].media_type==callmode){
                /*
                 printf("QuStreamResume:session id=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
                 qcall[id].session,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
                 */
                /*reset audio and video codec*/
                QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
                QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
                
                QStreamPara media;
                set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
                
                //!!!!!!!It is for QT
                if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                {
                    // Enter the critical section
#if MUTEX_DEBUG
                    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                    pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                    
                    qcall[id].qt_call->mCallData.is_reinvite = true;
                    qcall[id].qt_call->mCallData.is_recv_hold = false;
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_UNHOLD;
                    setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
                    g_event_listner->onCallIncoming(qcall[id].qt_call);
                    g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
                    
#if MUTEX_DEBUG
                    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                    pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                }
                //---------------------
            }else if (HOLD_BEHOLD==qcall[id].callstate){
                
                if (qcall[id].media_type==AUDIO){
                    int i;
                    int media_size;
                    pSessMedia sess_media=NULL;
                    MediaType media_type;
                    
                    /*2010-07-13 remove part of video*/
                    SessGetMediaSize(qcall[id].lsdp,&media_size);
                    
                    for (i=0;i<media_size;i++){
                        SessGetMedia(qcall[id].lsdp,i,&sess_media);
                        SessMediaGetType(sess_media,&media_type);
                        
                        if (media_type==MEDIA_VIDEO){
                            SessRemoveMedia(qcall[id].lsdp,media_size-1);
                        }
                    }
                }
                
                printf("HOLD_BEHOLD->HOLD\n");
                qcall[id].callstate = HOLD;
                
                printf("fw_inbound_resume :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
                
            }else if (ACTIVE == qcall[id].callstate || EMPTY == qcall[id].callstate){
                // 05/17add EMPTY == qcall[id].callstate
                // VoiceMode<==>VideoMode
                char remote_a_ip[32]="0";
                char remote_v_ip[32]="0";
                int raudio_port=0,rvideo_port=0;
                int callmode;
                
                QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
                if(0!=rvideo_port){
                    callmode=VIDEO;
                }else{
                    callmode=AUDIO;
                }
                
                //qcall[id].multiple_way_video = QuGetMultipleWayVideo( callmode );
                printf("callmode=%d,%d\n",qcall[id].media_type,callmode);
                
                if (qcall[id].media_type==callmode){
                    
                    
                    printf("Active 1\n");
                    char remoteip[32]="0";
                    int laudio_port=0,lvideo_port=0;
                    int raudio_port=0,rvideo_port=0;
                    
                    MediaCodecInfo media_info;
                    MediaCodecInfoInit(&media_info);
                    
                    QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
                    QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
                    QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
                    QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
                    QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
                    
                    laudio_port = qcall[id].audio_port;
                    lvideo_port = qcall[id].video_port;
                    /*
                     printf("QuStreamSwitchCallMode:session=%d,Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
                     qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
                     */
                    
                    /*2011-11-28 added by nicky*/
                    /*reset audio and video codec*/
                    QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
                    QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
                    
                    QStreamPara media;
                    set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);
                    
                    //!!!!!!!It is for QT AD-Hoc Test!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                    {

                        // Enter the critical section
#if MUTEX_DEBUG
                        printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                        
                        printf("!!!!!!!!!!!!!!!!onSessionEnd\n");
                        g_event_listner->onSessionEnd(qcall[id].qt_call);
                        
                        printf("!!!!!!!!!!!!!!!!onSessionStart\n");
                        g_event_listner->onSessionStart(qcall[id].qt_call);
                        
#if MUTEX_DEBUG
                        printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                    }
                    
                }else{
                    
                    /*2011-0915 added by nicky*/
                    /*check video resource , if system is busy , sip will auto switch to video*/
                    
                    char *bid;
                    bid=GetReqViaBranch(req);
                    //memcpy(qcall[id].reqid,bid,64);
                    qcall[id].reqid=strdup(bid);
                    //printf("%s\n",qcall[id].reqid);
                    qcall[id].dlg=dlg;
                    
                    printf("fw_inbound_request_switch:name=%s,phone=%s,mode=%d\n",displayname,phonenumber,callmode);
                    
                    
                    //2011-05-06
                    if(callmode==1) {
                        printf("response mode = SWITCH TO VIDEO\n");
                        qcall[id].response_mode=RESPONSE_SWITCH_TO_VIDEO;
                    } else if (callmode ==0) {
                        printf("response mode = SWITCH TO AUDIO\n");
                        qcall[id].response_mode=RESPONSE_SWITCH_TO_AUDIO;
                    }
                    
                    //!----For QT----//
                    
                    if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                    {
                        // Enter the critical section
#if MUTEX_DEBUG
                        printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                        
                        qcall[id].qt_call->mCallData.is_reinvite = true;
                        if (callmode==VIDEO)
                            qcall[id].qt_call->mCallData.is_video_call = true;
                        else
                            qcall[id].qt_call->mCallData.is_video_call = false;
                        
                        if(g_event_listner != NULL)
                            g_event_listner->onCallIncoming(qcall[id].qt_call);
                        
#if MUTEX_DEBUG
                        printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                    }
                    
                    return 1;
                }	
                
            }
        }
    }else{
        
        if(ccNewRspFromCall(&call_obj,&rsp,200,"ok",NULL,GetReqViaBranch(req),dlg)==RC_OK){
            printf("\n%s\n",sipRspPrint(rsp));
            /*2011-0617*/
            //session timer
            if(se)
                sipRspAddHdr(rsp,Session_Expires,(void*)se);
            
            ccSendRsp(&call_obj,rsp);
            sipRspFree(rsp);
        }

        g_inviteProgress=FALSE;
        qcall[id].g_RequestProgress=FALSE;
        
        //!----For QT----//
        qcall[id].qt_call->mCallData.is_reinvite = false;
        
        return 1;
  
    }
    
    StatusAttr status;
    if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){
        
        /*check media description attribut status*/
        if (SessGetStatus(qcall[id].rsdp,&status)==RC_OK){
            printf("sess status:%s\n",StatusAttrToStr(status));
            
            if (hs==CZEROS){
                if (SessSetHold(qcall[id].lsdp,CZEROS)==RC_OK){
                    SessSetStatus(qcall[id].lsdp,INACTIVE);
                    ;//printf("#######\n SessSetHold ERROR!!!!!\n");
                }
            }else if (SENDONLY==status){
                //if (SessSetUnHold(qcall[id].lsdp ,profile.ip)==RC_OK){
                SessSetStatus(qcall[id].lsdp,RECVONLY);
                //}
            }else if (RECVONLY==status){
                SessSetStatus(qcall[id].lsdp,SENDONLY);
            }
        }
        
        int media_size;
        pSessMedia media=NULL;
        MediaType media_type;
        int i;
        SessGetMediaSize(qcall[id].rsdp,&media_size);
        for ( i=0 ; i< media_size;i++){
            SessGetMedia(qcall[id].rsdp,i,&media);
            SessMediaGetType(media,&media_type);
            if (media_type==MEDIA_AUDIO){		
                if(SessMediaGetStatus(media,&status)==RC_OK){
                    if (SENDONLY==status){
                        printf("media status:%s\n",StatusAttrToStr(status));
                        SessSetHold(qcall[id].lsdp,AUDIO_RECVONLY);
                    }else if (RECVONLY==status){
                        printf("media status:%s\n",StatusAttrToStr(status));
                        SessSetHold(qcall[id].lsdp,AUDIO_SENDONLY);
                    }else if (INACTIVE==status){
                        printf("media status:%s\n",StatusAttrToStr(status));
                        SessSetHold(qcall[id].lsdp,AUDIO_INACTIVE);
                    }
                }
            }
        }
    }else{
        if (SessSetUnHold(qcall[id].lsdp ,profile.ip)==RC_OK){
            SessSetStatus(qcall[id].lsdp,SENDRECV);
            ;//printf("#######\n SessSetUnHold ERROR!!!!!\n");
        }
    }
    
    //reinvite SDP is null , send 200 ok with SDP
    sesslen=SESS_BUFF_LEN-1;
    SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
    ccContNew(&msgbody,"application/sdp",sessbuf);
    
    if(ccNewRspFromCall(&call_obj,&rsp,200,"ok",msgbody,GetReqViaBranch(req),dlg)==RC_OK){
        printf("\n%s\n",sipRspPrint(rsp));
        /*2011-0617*/
        //session timer
        if(se)
            sipRspAddHdr(rsp,Session_Expires,(void*)se);
        
        ccSendRsp(&call_obj,rsp);
        sipRspFree(rsp);
    }
    
    ccContFree(&msgbody);
    
    g_inviteProgress=FALSE;
    qcall[id].g_RequestProgress=FALSE;
    
    //!----For QT----//
    qcall[id].qt_call->mCallData.is_reinvite = false;
    
    return 1;
}

//Receive Re-Invite
int QuRevReInvite(pCCCall call_obj, SipReq req, pSipDlg dlg)
{
	int res=0;			
	char *displayname = GetReqFromDisplayname(req);
	if (!displayname){
		displayname=strdup("");
	}
	
	char phonenumber[64]="0";
	getPhoneFromReq(req,phonenumber);
	SipRsp rsp=NULL;
	pSess sess=NULL;
	HoldStyle hs = NOT_HOLD;
	char sessbuf[SESS_BUFF_LEN]="0";
	int sesslen=SESS_BUFF_LEN-1;
	pCont content=NULL;
	pCont msgbody=NULL;
	int id = QuGetCall(phonenumber);
	printf("id=%d\n",id);
	if (-1==id){
		return 0 ;
	}
    
	ccContNewFromReq(&content,req);
	
	UINT32 contlen=0;
	ccContGetLen(&content,&contlen);
	char *contbuf;
    
	//session timer
	SipSessionExpires *se=(SipSessionExpires *)sipReqGetHdr(req,Session_Expires);
    
	if (0 < contlen){

		contbuf = (char*) malloc (contlen+8);
		memset(contbuf,0, contlen+8);

		QuContToSessToStr(content,contbuf,&contlen);
		ccContFree(&content);

		unsigned long oldversion,newversion; 
		SessNewFromStr(&sess,contbuf);
		SessGetVersion(sess,&newversion);
		SessFree(&sess);
		
		SessGetVersion(qcall[id].rsdp,&oldversion);
		printf("old version =%lu , new version =%lu\n",oldversion,newversion);
		if (newversion <= oldversion){
		//if (newversion == oldversion){
			/*session audit*/
			if (newversion==oldversion){
				sesslen=1023;
				SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
				ccContNew(&msgbody,"application/sdp",sessbuf);
				
				if(ccNewRspFromCall(&call_obj,&rsp,200,"ok",msgbody,GetReqViaBranch(req),dlg)==RC_OK){
					ccSendRsp(&call_obj,rsp);
					sipRspFree(rsp);
				}
				ccContFree(&msgbody);
			}

			free(contbuf);
			g_inviteProgress=FALSE;
			return 0 ;
		}
    
		//memcpy(&qcall[id].rsdp ,sess , sizeof(sess));//save new sdp
		if (NULL!=qcall[id].rsdp){
			SessFree(&(qcall[id].rsdp));
		}
		SessNewFromStr(&qcall[id].rsdp,contbuf);
		free(contbuf);
		
		SessCheckHold(qcall[id].rsdp,&hs);
		if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){ 
			printf("It's a hold sdp\n");
			
			g_inviteProgress=TRUE;
			qcall[id].g_RequestProgress=TRUE;
			
			if(qcall[id].request_mode==REQUEST_SWITCH_TO_AUDIO || qcall[id].request_mode==REQUEST_SWITCH_TO_VIDEO) {
			    printf("Hold > Switch, cancel the switch request!\n");
			    
			    int switch_mode;
			    if (REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode){
				    switch_mode = AUDIO;
			    }else if (REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode){
				    switch_mode = VIDEO;
			    }
			    
			    qcall[id].request_mode=REQUEST_EMPTY;
			}
			
			printf("fw_inbound_hold :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
			
            //!----For QT----//
            
            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
            {
                // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                
                qcall[id].qt_call->mCallData.is_recv_hold = true;
                qcall[id].qt_call->mCallData.call_state = CALL_STATE_HOLD;
                qcall[id].qt_call->mCallData.is_reinvite = true;
                
                printf("g_event_listner->onCallIncoming %d\n",qcall[id].qt_call);
                g_event_listner->onCallIncoming(qcall[id].qt_call);
                g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, ""); 
                printf("id=%d,qt=%d,state=%d\n",id,qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state);
                
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                //----------------------
            }
			QuReInviteOfferToAnswer(id,qcall[id].rsdp);
			
			if (HOLD != qcall[id].callstate && HOLD_BEHOLD != qcall[id].callstate && qcall[id].request_mode!=REQUEST_HOLD){
				qcall[id].callstate = BEHOLD;
#if 0				
				if (hs==CZEROS || AUDIO_INACTIVE==hs){
					//QStreamPara media;
					//set_media_para(&media,qcall[id].session,-1,NULL,NULL,-1,-1,-1,-1,NULL,-1,0);
				
				}else{

					printf("hold state,but not CZEROS or AUDIO_INACTIVE\n");
					char remote_a_ip[32]="0";
					char remote_v_ip[32]="0";
					int laudio_port=0,lvideo_port=0;
					int raudio_port=0,rvideo_port=0;
					
					MediaCodecInfo media_info;
					MediaCodecInfoInit(&media_info);
					
					int callmode;
	
					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
					QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;
					
					if(0!=rvideo_port){
						callmode=VIDEO;
					}else{
						callmode=AUDIO;
					}
					/*
					printf("QuStreamSwitchCallMode:session=%d,Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
					qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
		
					*/
					printf("id=%d, phone=%s\n",id,qcall[id].phone);
					QStreamPara media;
					set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);					
				}
#endif				
			}else{
				printf("qcall[id].callstate = HOLD_BEHOLD\n");
				qcall[id].callstate = HOLD_BEHOLD;
			}
		}else{
			printf("QuRevInvite:Not Hold SDP\n");
			unsigned long oldversion,newversion;
			unsigned long oldid,newid; 
	
			//SessGetVersion(qcall[id].lsdp,&oldversion);
			//SessGetSessID(qcall[id].lsdp,&oldid);
			printf("request mode = %d, callstate=%d , g_inviteProgress=%d\n",qcall[id].request_mode,qcall[id].callstate,g_inviteProgress);
			if (qcall[id].request_mode==REQUEST_HOLD || HOLD==qcall[id].callstate || (g_inviteProgress && BEHOLD!=qcall[id].callstate && HOLD_BEHOLD!=qcall[id].callstate)) {
				/*if callstate==hold , receiving swicting request will auto reject(406)*/
				if (BEHOLD!=qcall[id].callstate && HOLD_BEHOLD!=qcall[id].callstate){
					pCont msgbody=NULL;
					SipRsp rsp=NULL;
					char sessbuf[SESS_BUFF_LEN]="0";
					int sesslen=SESS_BUFF_LEN-1;
					//SessAddVersion(qcall[id].lsdp);//add sdp version	
					SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
					ccContNew(&msgbody,"application/sdp",sessbuf);
							
					if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,406,"Not Accept",NULL,GetReqViaBranch(req),dlg)==RC_OK){
						printf("callstate==hold,auto reject switch\n");
						printf("request_mode=%d\n",qcall[id].request_mode);
						printf("callstate=%d\n",qcall[id].callstate);
					}
					ccSendRsp(&qcall[id].call_obj,rsp);
					sipRspFree(rsp);
					ccContFree(&msgbody);
					return 1;
				}
			}else if (ACTIVE == qcall[id].callstate || EMPTY == qcall[id].callstate){
				/*if already send request , receiving swicting request will auto reject(406)*/
				if ((REQUEST_HOLD==qcall[id].request_mode)||(REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode)||(REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode)){
					pCont msgbody=NULL;
					SipRsp rsp=NULL;
					char sessbuf[SESS_BUFF_LEN]="0";
					int sesslen=SESS_BUFF_LEN-1;
					//SessAddVersion(qcall[id].lsdp);//add sdp version	
					SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
					ccContNew(&msgbody,"application/sdp",sessbuf);
							
					if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,406,"Not Accept",NULL,GetReqViaBranch(req),dlg)==RC_OK){
						printf("request_mode==REQUEST_HOLD||REQUEST_SWITCH_TO_AUDIO||REQUEST_SWITCH_TO_VIDEO,auto reject switch\n");
						printf("request_mode=%d\n",qcall[id].request_mode);
					}
					ccSendRsp(&qcall[id].call_obj,rsp);
					sipRspFree(rsp);
					ccContFree(&msgbody);
					
					return 1;
				}
			}
			
			
			g_inviteProgress=TRUE;
			qcall[id].g_RequestProgress=TRUE;
			/*2010-0621*/
			QuReInviteOfferToAnswer(id,qcall[id].rsdp);
			
			//SessGetVersion(qcall[id].lsdp,&newversion);
			//SessGetSessID(qcall[id].lsdp,&newid);
			//printf("old version =%lu , new version =%lu\n",oldversion,newversion);
			//printf("old id =%ld , new id =%ld\n",oldid,newid);
			
			printf("qcall[id].callstate=%d\n",qcall[id].callstate);

			if (BEHOLD==qcall[id].callstate){ /*2011/7/15 marked by nicky || HOLD==qcall[id].callstate*/
				
				qcall[id].callstate = ACTIVE;
				
				char remote_a_ip[32]="0";
				char remote_v_ip[32]="0";
				int laudio_port=0,lvideo_port=0;
				int raudio_port=0,rvideo_port=0;
				
				MediaCodecInfo media_info;
				MediaCodecInfoInit(&media_info);
				
				int callmode;
				
				if (qcall[id].media_type==AUDIO){
					int i;
					int media_size;
					pSessMedia sess_media=NULL;
					MediaType media_type;
					
					/*2010-07-13 remove part of video*/
					SessGetMediaSize(qcall[id].lsdp,&media_size);
				}
				
				QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
				QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
				QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
				QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
				QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

				laudio_port = qcall[id].audio_port;
				lvideo_port = qcall[id].video_port;
				
				if(0!=rvideo_port){
					callmode=VIDEO;
				}else{
					callmode=AUDIO;
				}
				
				printf("fw_inbound_resume :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
				
			    //if (qcall[id].media_type==callmode){
			    /*
			    printf("QuStreamResume:session id=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
				    qcall[id].session,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
			    */
				/*reset audio and video codec*/
				QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
				QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec); 
			   
				QStreamPara media;
			    set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
				
				//!!!!!!!It is for QT
                if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                {
                    // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                    pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                    
                    qcall[id].qt_call->mCallData.is_reinvite = true;
                    qcall[id].qt_call->mCallData.is_recv_hold = false;
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_UNHOLD;
                    setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
                    g_event_listner->onCallIncoming(qcall[id].qt_call);
                    g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
                    
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                    pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                }
				//---------------------
			}else if (HOLD_BEHOLD==qcall[id].callstate){
				
				if (qcall[id].media_type==AUDIO){
					int i;
					int media_size;
					pSessMedia sess_media=NULL;
					MediaType media_type;
					
					/*2010-07-13 remove part of video*/
					SessGetMediaSize(qcall[id].lsdp,&media_size);
					
					for (i=0;i<media_size;i++){
						SessGetMedia(qcall[id].lsdp,i,&sess_media);
						SessMediaGetType(sess_media,&media_type);
						
						if (media_type==MEDIA_VIDEO){
							SessRemoveMedia(qcall[id].lsdp,media_size-1);
						}
					}
				}
				
				printf("HOLD_BEHOLD->HOLD\n");	
				qcall[id].callstate = HOLD;
				
				printf("fw_inbound_resume :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
			
			}else if (ACTIVE == qcall[id].callstate || EMPTY == qcall[id].callstate){
				// 05/17add EMPTY == qcall[id].callstate  
				// VoiceMode<==>VideoMode
				char remote_a_ip[32]="0";
				char remote_v_ip[32]="0";
				int raudio_port=0,rvideo_port=0;
				int callmode;
				
				QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
				if(0!=rvideo_port){
					callmode=VIDEO;
				}else{
					callmode=AUDIO;
				}
				
				//qcall[id].multiple_way_video = QuGetMultipleWayVideo( callmode );
				
				if (qcall[id].media_type==callmode){
					char remoteip[32]="0";
					int laudio_port=0,lvideo_port=0;
					int raudio_port=0,rvideo_port=0;
					
					MediaCodecInfo media_info;
					MediaCodecInfoInit(&media_info);

					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
					QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;			
					/*
					printf("QuStreamSwitchCallMode:session=%d,Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
					qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
					*/
					
					/*2011-11-28 added by nicky*/
					/*reset audio and video codec*/
					QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
					QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
					
					QStreamPara media;
					set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);

				}else{
				
					/*2011-0915 added by nicky*/
					/*check video resource , if system is busy , sip will auto switch to video*/
				
					char *bid;
					bid=GetReqViaBranch(req);
					//memcpy(qcall[id].reqid,bid,64);
					qcall[id].reqid=strdup(bid);
					//printf("%s\n",qcall[id].reqid);
					qcall[id].dlg=dlg;
					
					printf("fw_inbound_request_switch:name=%s,phone=%s,mode=%d\n",displayname,phonenumber,callmode);
					
                    
                    //2011-05-06
					if(callmode==1) {
                        printf("response mode = SWITCH TO VIDEO\n");
                        qcall[id].response_mode=RESPONSE_SWITCH_TO_VIDEO;
					} else if (callmode ==0) {
						printf("response mode = SWITCH TO AUDIO\n");
						qcall[id].response_mode=RESPONSE_SWITCH_TO_AUDIO;
					}
                    
					//!----For QT----//
                    
                    if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                    {
                        // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
					
                        qcall[id].qt_call->mCallData.is_reinvite = true;
                        if (callmode==VIDEO)
                            qcall[id].qt_call->mCallData.is_video_call = true;
                        else
                            qcall[id].qt_call->mCallData.is_video_call = false;

                        /*
                        if(g_event_listner != NULL)
                            g_event_listner->onCallIncoming(qcall[id].qt_call);
                        */
                        
                        sip_confirm_switch(0, 1, qcall[id].phone);

#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                    }

					return 1;
				}	
				
			}
		}
	}else{
		
		pSessMedia audio_media=NULL;
		pSessMedia video_media=NULL;
		int media_size,i;
		//clear old sdp
		SessGetMediaSize(qcall[id].lsdp,&media_size);
		printf("re-invite without SDP.clear old sdp\n");
		for (i=0;i<media_size;i++){
			SessRemoveMedia(qcall[id].lsdp,0);
		}
		//generate SDP	
		GenAudioMedia(&audio_media);
			
		if (qcall[id].media_type==VIDEO){
			/*2010-08-04*/
			//generate SDP	
			GenVideoMedia(&video_media);
		}
			
		//add new audio media
		SessAddMedia(qcall[id].lsdp,audio_media);
			
		//if (qcall[id].media_type==VIDEO){
			//add new video media
			SessAddMedia(qcall[id].lsdp,video_media);
		//}
		
        /*set new address */
        SessSetAddress(qcall[id].lsdp, profile.ip);
        SessSetOwnerAddress(qcall[id].lsdp, profile.ip);
        
		//add sdp version
		SessAddVersion(qcall[id].lsdp);	
	}
	
	StatusAttr status;
    
    printf("re-invite check status=%d\n",hs);
    
	if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){
		
		/*check media description attribut status*/
		if (SessGetStatus(qcall[id].rsdp,&status)==RC_OK){
			printf("sess status:%s\n",StatusAttrToStr(status));
			
			if (hs==CZEROS){
				if (SessSetHold(qcall[id].lsdp,CZEROS)==RC_OK){
					SessSetStatus(qcall[id].lsdp,INACTIVE);
					;//printf("#######\n SessSetHold ERROR!!!!!\n");
				}
			}else if (SENDONLY==status){
				//if (SessSetUnHold(qcall[id].lsdp ,profile.ip)==RC_OK){
					SessSetStatus(qcall[id].lsdp,RECVONLY);
				//}
			}else if (RECVONLY==status){
				SessSetStatus(qcall[id].lsdp,SENDONLY);
			}
		}
		
		int media_size;
		pSessMedia media=NULL;
		MediaType media_type;
		int i;
		SessGetMediaSize(qcall[id].rsdp,&media_size);
		for ( i=0 ; i< media_size;i++){
			SessGetMedia(qcall[id].rsdp,i,&media);
			SessMediaGetType(media,&media_type);
			if (media_type==MEDIA_AUDIO){		
				if(SessMediaGetStatus(media,&status)==RC_OK){
					if (SENDONLY==status){
						printf("media status:%s\n",StatusAttrToStr(status));
						SessSetHold(qcall[id].lsdp,AUDIO_RECVONLY);
					}else if (RECVONLY==status){
						printf("media status:%s\n",StatusAttrToStr(status));
						SessSetHold(qcall[id].lsdp,AUDIO_SENDONLY);
					}else if (INACTIVE==status){
						printf("media status:%s\n",StatusAttrToStr(status));
						SessSetHold(qcall[id].lsdp,AUDIO_INACTIVE);
					}
				}
			}
		}
	}else{
		if (SessSetUnHold(qcall[id].lsdp ,profile.ip)==RC_OK){
			SessSetStatus(qcall[id].lsdp,SENDRECV);
			;//printf("#######\n SessSetUnHold ERROR!!!!!\n");
		}
	}
	
	//reinvite SDP is null , send 200 ok with SDP
	sesslen=SESS_BUFF_LEN-1;
	SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
	ccContNew(&msgbody,"application/sdp",sessbuf);
	
	if(ccNewRspFromCall(&call_obj,&rsp,200,"ok",msgbody,GetReqViaBranch(req),dlg)==RC_OK){
		//printf("\n%s\n",sipRspPrint(rsp));
		/*2011-0617*/
		//session timer
		if(se)
			sipRspAddHdr(rsp,Session_Expires,(void*)se);
		
		ccSendRsp(&call_obj,rsp);
		sipRspFree(rsp);
	}
	
	ccContFree(&msgbody);
	
	g_inviteProgress=FALSE;
	qcall[id].g_RequestProgress=FALSE;

	//!----For QT----//
	qcall[id].qt_call->mCallData.is_reinvite = false;

	return 1;
}

int QuRevReInvite200OK(pCCCall call,SipRsp rsp){
	int res=0;
	HoldStyle hs;
	pCont content=NULL;
	pSess sess=NULL;
	char *displayname = GetRspToDisplayname(rsp);
	if (!displayname){
		displayname=strdup("");
	}
	char phonenumber[64]="0";
	getPhoneToRsp(rsp,phonenumber);
	int id=QuGetCall(phonenumber);

	
	printf("Receive reinvite 200OK %s\n",phonenumber);
	if (-1==id){
		return 0 ;
	}
	
	unsigned long oldversion,newversion;
	
	ccContNewFromRsp(&content,rsp);
	
	UINT32 contlen=0;
	ccContGetLen(&content,&contlen);
	char *contbuf;
	
	if (0 < contlen){

		contbuf = (char*) malloc (contlen+8);
		memset(contbuf,0, contlen+8);

		QuContToSessToStr(content,contbuf,&contlen);
		ccContFree(&content);
		content=NULL;

		unsigned long oldid,newid; 
		SessNewFromStr(&sess,contbuf);
		SessGetVersion(sess,&newversion);
		SessGetSessID(sess,&newid);
		SessFree(&sess);
		
		SessGetVersion(qcall[id].rsdp,&oldversion);
		SessGetSessID(qcall[id].rsdp,&oldid);
		printf("old version =%lu , new version =%lu\n",oldversion,newversion);
		printf("old id =%ld , new id =%ld\n",oldid,newid);
		
		/*check sdp version is not equal*/
		//if ((newversion != oldversion) || qcall[id].request_mode==REQUEST_SWITCH_TO_AUDIO || qcall[id].request_mode==REQUEST_SWITCH_TO_VIDEO){
		if ((newversion > oldversion)){
			if (NULL!=qcall[id].rsdp){
				SessFree(&(qcall[id].rsdp));
			}
			SessNewFromStr(&qcall[id].rsdp,contbuf);
			free(contbuf);
			
			if (check_answer_completely(qcall[id].rsdp)){
				int sesslen=SESS_BUFF_LEN-1;
				char sessbuf[SESS_BUFF_LEN]="0";
				QuReInviteOfferToAnswer(id,qcall[id].rsdp);
			}
			SessCheckHold(qcall[id].rsdp,&hs);
			
			if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){ 
				printf("It's a hold sdp\n");
				
				if (ACTIVE == qcall[id].callstate){
					qcall[id].callstate = HOLD;
				}else if (BEHOLD == qcall[id].callstate){
					printf("reinvite 200OK BEHOLD -> HOLD_BEHOLD\n");
					qcall[id].callstate=HOLD_BEHOLD;
				}else if (HOLD_BEHOLD == qcall[id].callstate){
					printf("reinvite 200OK HOLD_BEHOLD -> HOLD_BEHOLD\n");
					qcall[id].callstate=HOLD_BEHOLD;
				}
			}else{
			
			//if (hs!=CZEROS && hs!=AUDIO_SENDONLY && hs!=AUDIO_RECVONLY && hs!=AUDIO_INACTIVE){ 
				printf("QuRevReInvite200OK:Not Hold SDP\n");
				printf("qcall[id].callstate=%d\n",qcall[id].callstate);
				if (HOLD == qcall[id].callstate || BEHOLD == qcall[id].callstate){
					
					qcall[id].callstate = ACTIVE;
					
					char remote_a_ip[32]="0";
					char remote_v_ip[32]="0";
					int laudio_port=0,lvideo_port=0;
					int raudio_port=0,rvideo_port=0;
					
					MediaCodecInfo media_info;
					MediaCodecInfoInit(&media_info);

					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
					/*2011-0315 nicky*/
					/*change get codec information from lsdp to rsdp*/
					if (check_answer_completely(qcall[id].rsdp)){
						QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
					}else{
						QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
					}
						
					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;
					
					if (BEHOLD == qcall[id].callstate){

						qcall[id].callstate = HOLD;
					}
					
					QStreamPara media;
					set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
					
					// for QT
					setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
                    
					//printf("State = %d  g_isQstreamConference=%d \n",QuSetIsVideoConference(id,qcall[id].rsdp,NULL), g_isQstreamConference);
					
				}else if (HOLD_BEHOLD==qcall[id].callstate){
						printf("HOLD_BEHOLD->BEHOLD\n");	
						qcall[id].callstate = BEHOLD;
						printf("fw_inbound_hold :displayname=%s,phonenumber=%s\n",displayname,phonenumber);

				}else if (ACTIVE == qcall[id].callstate){// VoiceMode<==>VideoMode
					
					char remote_a_ip[32]="0";
					char remote_v_ip[32]="0";
					int laudio_port=0,lvideo_port=0;
					int raudio_port=0,rvideo_port=0;
					
					MediaCodecInfo media_info;
					MediaCodecInfoInit(&media_info);
					
					int callmode;
					
					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
				
					/*change get codec information from lsdp to rsdp*/
					if (check_answer_completely(qcall[id].rsdp)){
						printf("!!Qurevreinvite200ok check_answer_completely=true\n");
						QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
					}else{
						printf("!!Qurevreinvite200ok check_answer_completely=false\n");
						QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
					}
					
					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					if((0!=rvideo_port)&&(-1!=media_info.video_payload_type)){
						callmode=VIDEO;
					}else{
						callmode=AUDIO;
					}
									
					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;
					
					if ((REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode)||(REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode)){
						
						int switch_mode;
						if (REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode){
							switch_mode = AUDIO;
						}else if (REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode){
							switch_mode = VIDEO;
						}
						
						if (switch_mode==callmode){
							printf("fw_inbound_confirm_switch:accept=%d,name=%s,phone=%s,mode=%d\n",1,displayname,phonenumber,callmode);
							qcall[id].media_type=callmode;
							
							/*
							printf("QuStreamSwitchCallMode:session=%d,Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
							qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
							*/
											
							QStreamPara media;
							set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);
							
							//!----for QT----//
                            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                            {
                                // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                                
                                setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
                                printf("is_session_change = true\n");
                                qcall[id].qt_call->mCallData.is_session_change = true;
                                //g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, ""); 
							
                                g_event_listner->onSessionEnd(qcall[id].qt_call);
                                g_event_listner->onSessionStart(qcall[id].qt_call);
                                
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                            }

						}else{
							printf("fw_inbound_confirm_switch:%d %s %s %d\n",0,displayname,phonenumber,switch_mode);
							
							/*1:accetp 0:not accept. 486=>not accept*/
							res=0;
							//printf("\n\n\n\n\nswitch_mode = %d\n",switch_mode);
					
							QStreamPara media;
							set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);

						}
							
						
					}else if (qcall[id].wait_response== WAITRESPONSE_HOLD){
						//2011/11/07
						//send re-invite hold , but receive 200ok not hold sdp, resume qstream and cancel holds
						printf("wait_response = WAITRESPONSE_HOLD\n");
						if (ACTIVE == qcall[id].callstate){
							printf("qcall[id].callstate = HOLD\n");
							qcall[id].callstate = HOLD;
						}else if (BEHOLD == qcall[id].callstate){
							printf("qcall[id].callstate = HOLD_BEHOLD\n");
							qcall[id].callstate=HOLD_BEHOLD;
						}
						
					}else{
						
					}
				}
			}
		
			qcall[id].request_mode=REQUEST_EMPTY;
			qcall[id].wait_response=WAITRESPONSE_EMPTY;
	
		}else if (newversion == oldversion){//sdp new version = old version
			if (qcall[id].wait_response== WAITRESPONSE_HOLD){
				//send re-invite hold , but receive 200ok not hold sdp, resume qstream and cancel holds
				printf("wait_response = WAITRESPONSE_HOLD\n");
				if (ACTIVE == qcall[id].callstate){
					printf("qcall[id].callstate = HOLD\n");
					qcall[id].callstate = HOLD;
				}else if (BEHOLD == qcall[id].callstate){
					printf("qcall[id].callstate = HOLD_BEHOLD\n");
					qcall[id].callstate=HOLD_BEHOLD;
				}
			}else if (qcall[id].wait_response== WAITRESPONSE_RESUME){
				if (HOLD == qcall[id].callstate || BEHOLD == qcall[id].callstate){
					
					printf("the same SDP version\n");
					printf("qcall[%d].callstate = %d\n",id,qcall[id].callstate);
					qcall[id].callstate = ACTIVE;
					
					char remote_a_ip[32]="0";
					char remote_v_ip[32]="0";
					int laudio_port=0,lvideo_port=0;
					int raudio_port=0,rvideo_port=0;
					
					MediaCodecInfo media_info;
					MediaCodecInfoInit(&media_info);

					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
					/*2011-0315 nicky*/
					/*change get codec information from lsdp to rsdp*/
					
					if (check_answer_completely(qcall[id].rsdp)){
						printf("!!Qurevreinvite200ok check_answer_completely=true\n");
						QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
					}else{
						printf("!!Qurevreinvite200ok check_answer_completely=false\n");
						QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
					}
					
					//QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;
					if (BEHOLD == qcall[id].callstate){

						qcall[id].callstate = HOLD;
					}
					
					QStreamPara media;
					set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);	
					//printf("State = %d  g_isQstreamConference=%d \n",QuSetIsVideoConference(id,qcall[id].rsdp,NULL), g_isQstreamConference);
					
				}
			}
			/* move free request_mode and wait_response after checking version*/
			qcall[id].request_mode=REQUEST_EMPTY;
			qcall[id].wait_response=WAITRESPONSE_EMPTY;
		}
	}
	ccAckCall(&call,content);
	g_inviteProgress=FALSE;
	
    //!----For QT----//
	qcall[id].qt_call->mCallData.is_reinvite = false;
    
	return 1;
}

void QuReInviteOfferToAnswer(int id , pSess offer){
	int media_size;
	pMediaPayload payload=NULL;
	UINT32 rtpmap_size;
	int i,j;
	MediaType media_type;
	pSessMedia media=NULL;
	pSessMedia answer_media=NULL;
	pMediaPayload fmtp_payload=NULL;
	
	char payloadbuf[256]="0";
	int payloadlen=255;
	
	char payloadtype[32]="0";
	int payloadtypelen;
	
	bool is_video_match=false;
	bool is_video_add_session=false;
	bool is_audio_match=false;
	bool is_audio_add_session=false;
	
	char audio_priority[NUMOFAUDIOCODEC][16];
	
	int priority[NUMOFAUDIOCODEC];
	
	memset(audio_priority[0],0,sizeof(audio_priority[0]));
	strcpy(audio_priority[0],sip_config.qtalk.sip.audio_codec_preference_1);
	//strcpy(audio_priority[0],OPUS_NAME);

	memset(audio_priority[1],0,sizeof(audio_priority[1]));
	strcpy(audio_priority[1],sip_config.qtalk.sip.audio_codec_preference_2);
	//strcpy(audio_priority[1],PCMU_NAME);

	memset(audio_priority[2],0,sizeof(audio_priority[2]));
	strcpy(audio_priority[2],sip_config.qtalk.sip.audio_codec_preference_3);
	//strcpy(audio_priority[2],PCMA_NAME);
	/*
	memset(audio_priority[3],0,sizeof(audio_priority[3]));
	strcpy(audio_priority[3],G722_NAME);
	
	*/

	//clear old sdp
	SessGetMediaSize(qcall[id].lsdp,&media_size);
	//printf("SessGetOldSdpMediaSize=%d\n",media_size);
	for (i=0;i<media_size;i++){
		SessRemoveMedia(qcall[id].lsdp,0);
	}
	
	SessGetMediaSize(offer,&media_size);
	printf("SessGetMediaSize=%d\n",media_size);
	for ( i=0 ; i< media_size;i++){
		SessGetMedia(offer,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		printf("rtpmap_size=%d\n",rtpmap_size);
		if (media_type==MEDIA_AUDIO){
			int k=0;
			//2011-12-02 Choose the in-session codec first
			
			char in_session_audio_codec[8];
			QuAudioIntToCodec(qcall[id].audio_codec,in_session_audio_codec);
	
			for(j=0;j<rtpmap_size;j++){
				if (!is_audio_match){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
	
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
						char payloadtmp[40]="0";
						strcpy (payloadtmp,payloadbuf);
						printf("%s\n",payloadtmp);
						char *pch = strtok (payloadtmp,"/");
						int audio_type = atoi(payloadtype);
						if (!strcasecmp(PCMU_NAME,in_session_audio_codec) && 0==audio_type ){
							printf("set PCMU ok!\n");
							is_audio_match=true;
							break;
						}else if (!strcasecmp(PCMA_NAME,in_session_audio_codec) && 8==audio_type ){
							printf("set PCMA ok!\n");
							is_audio_match=true;
							break;
						}else if (!strcasecmp(G722_NAME,in_session_audio_codec) && 9==audio_type ){
							printf("set G722 ok!\n");
							is_audio_match=true;
							break;
						}else if (!strcasecmp(pch,in_session_audio_codec)){
								
							pch = strtok(NULL,"");
								
							if (!strcasecmp(G7221_NAME,in_session_audio_codec)){
								printf("G7221 payloadtype=%s\n",payloadtype);
								if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
									payloadlen=255;
									MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
									char fmtp_tmp[40]="0";
									strcpy (fmtp_tmp,payloadbuf);
									char *fmtp_pch = strtok (fmtp_tmp,"=");
									fmtp_pch = strtok(NULL,"");
									if (!strcmp(audio_codec[G7221].sample_rate,pch) && !strcmp(audio_codec[G7221].fmap,fmtp_pch)){
										printf("sample rate=%s,bitrate=%s\n",pch,fmtp_pch);
										printf("set %s ok!\n",audio_priority[k]);
										is_audio_match=true;
										break;
									}
										
								}
							}else if (!strcasecmp(OPUS_NAME,in_session_audio_codec)){
								printf("set OPUS ok!\n");
								is_audio_match=true;
								break;
							}
							
						}							
					}
						
				}
			}
			
			//If the in-session codec is not supported, follow the caller's codec priority
			for(j=0;j<rtpmap_size;j++){
				if (!is_audio_match){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
	
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
						//char *slash=strchr(payloadbuf,'/');
						char payloadtmp[40]="0";
						strcpy (payloadtmp,payloadbuf);
						printf("%s\n",payloadtmp);
						char *pch = strtok (payloadtmp,"/");
						int audio_type = atoi(payloadtype);
						for(k=0;k<NUMOFAUDIOCODEC;k++){
							if (!strcasecmp(PCMU_NAME,audio_priority[k]) && 0==audio_type ){
								printf("set PCMU ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(PCMA_NAME,audio_priority[k]) && 8==audio_type ){
								printf("set PCMA ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(G722_NAME,audio_priority[k]) && 9==audio_type ){
								printf("set G722 ok!\n");
								is_audio_match=true;
								break;
							}else if (!strcasecmp(pch,audio_priority[k])){
									
								pch = strtok(NULL,"");
									
								if (!strcasecmp(G7221_NAME,audio_priority[k])){
									printf("G7221 payloadtype=%s\n",payloadtype);
									if(SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload)==RC_OK){
										payloadlen=255;
										MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
										//char *pch = strtok (payloadtmp,"/");
										//printf("%s\n",payloadbuf);
										char fmtp_tmp[40]="0";
										strcpy (fmtp_tmp,payloadbuf);
										char *fmtp_pch = strtok (fmtp_tmp,"=");
										//printf("%s\n",fmtp_pch);
										fmtp_pch = strtok(NULL,"");
										//printf("%s\n",fmtp_pch);
										/*
										printf("pch=%s\n",pch);
										pch = strtok(NULL,"");
										printf("pch=%s\n",pch);
										*/
										if (!strcmp(audio_codec[G7221].sample_rate,pch) && !strcmp(audio_codec[G7221].fmap,fmtp_pch)){
											printf("sample rate=%s,bitrate=%s\n",pch,fmtp_pch);
											printf("set %s ok!\n",audio_priority[k]);
											is_audio_match=true;
											break;
										}
											
									}
								}else if (!strcasecmp(OPUS_NAME,audio_priority[k])){
									printf("set OPUS ok!\n");
									is_audio_match=true;
									break;
								}
								
							}

						}	
							
					}
						
				}
			}
			if (!is_audio_match){
				printf("!!!!!\nNo match audio_codec ok!\n");
			}
		}else if (media_type==MEDIA_VIDEO){
			//bool is_video_match=false;
			bool force_select_first_priority = false;
			int h264_rtpmap_size = 0;
			for(j=0;j<rtpmap_size;j++){
					
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
						
						char *pch = strtok (payloadtmp,"/");
						if (!strcasecmp(pch,"H264")){
							h264_rtpmap_size++;
						}
					}
			}
			
            printf("h264_rtpmap_size%d\n",h264_rtpmap_size,j);
            
            if(!is_video_match){
                for(j=0;j<rtpmap_size;j++){
                    printf("rtpmap_size=%d, j=%d\n",rtpmap_size,j);
                    if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
                        
                        payloadtypelen=31;
                        MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
                        payloadlen=255;
                        MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
                        char payloadtmp[128]="0";
                        strcpy (payloadtmp,payloadbuf);
                        
                        char *pch = strtok (payloadtmp,"/");
                        printf("%s\n",payloadbuf);
                        
                        if (!strcasecmp(pch,"H264")){
                            
                            //rtpmap_size =1 , do not check packetization mode
                            if (h264_rtpmap_size==1 || force_select_first_priority){
                                printf("Set VideoCodec H264 ok!\n");
                                is_video_match=true;
                                break;
                            }else{
                                
                                MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
                                pMediaPayload fmap_payload=NULL;
                                SessMediaGetRTPParam(media,FMTP,payloadtype,&fmap_payload);
                                
                                QuH264Parameter h264_parameter;
                                memset(&h264_parameter,0,sizeof(QuH264Parameter));
                                QuParseH264Parameter(fmap_payload,&h264_parameter);
                                if (h264_parameter.packetization_mode == 0){
                                    printf("Check pkt_mode,Set VideoCodec H264 ok!\n");
                                    is_video_match=true;
                                    break;
                                }
                                
                                //select the last video media , but no one set pkt-mode , need select first H264 payload 
                                printf("j=%d,rtpmap_size=%d\n",j,rtpmap_size);
                                if (j == rtpmap_size-1){
                                    printf("force_select_first_priority\n");
                                    force_select_first_priority=true;
                                    j=-1;
                                }
                            }
                        }
                        //payload = NULL;
                    }
                }
            }
			if (!is_video_match){
				printf("!!!!!\nNo match video_codec ok!\n");
				break;
			}
        }else if (media_type==MEDIA_APPLICATION){
            printf("media_type==MEDIA_APPLICATION\n");
            payload=NULL;
        }else if (media_type==MEDIA_IMAGE){
            printf("media_type==MEDIA_IMAGE\n");
            payload=NULL;
        }else if (media_type==MEDIA_MESSAGE){
            printf("media_type==MEDIA_MESSAGE\n");
            payload=NULL;
        }else{
            printf("media_type no match\n");
            payload=NULL;
        }
		
		if (NULL!=payload){
			int port;
			int ptime;
			if (media_type==MEDIA_AUDIO){
				//port = LAUDIOPORT;
				port = qcall[id].audio_port;
				ptime=20;
			}else if (media_type==MEDIA_VIDEO  && !is_video_add_session ){
				UINT32 rvideo_port;
				SessMediaGetPort(media,&rvideo_port);
				if (0==rvideo_port){//remote video port=0, local most response local video port 0
					//port=0;
                    port = qcall[id].video_port;
				}else{
					//port = LVIDEOPORT;
					port = qcall[id].video_port;
				}
				ptime=20;				
			}
			//SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,payload);
			pMediaPayload matchpaylod=NULL;
			MediaPayloadDup(payload,&matchpaylod);

			payloadlen=255;
			payloadtypelen=31;
	
			MediaPayloadGetParameter(matchpaylod,payloadbuf,&payloadlen);
			MediaPayloadGetType(matchpaylod,payloadtype,&payloadtypelen);
			
			//printf("%s,%s\n",payloadbuf,payloadtype);
						
			SessMediaNew(&answer_media,media_type,NULL,port,"RTP/AVP",ptime,RTPMAP,matchpaylod);
		
			/*********FMAP Test*******/
			/*2010-05-19*/
			/*2010-05-19*/
			if(RC_OK==SessMediaGetRTPParam(media,FMTP,payloadtype,&payload)){
				pMediaPayload matchfmap=NULL;
				/*2011-03-14*/
				if(media_type==MEDIA_VIDEO && !is_video_add_session) {
				  QuVideoFmtpAnswer(id,payload,payloadtype,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);
					/*2011-06-24 add bandwidth value*/
					int tias_bandwidth = media_bandwidth * 1000;
					SessMediaAddBandwidth(answer_media,"TIAS",tias_bandwidth);
				}
				else if(media_type==MEDIA_AUDIO){
				  MediaPayloadDup(payload,&matchfmap);
				  SessMediaAddRTPParam(answer_media,FMTP,matchfmap);
				}
			}
			/***************/
			
			/*********rtcp-fb*******/			
			
			if(media_type==MEDIA_VIDEO) { 
				if (QuSessMediaGetAttribute(media,"rtcp-fb","pli")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* nack pli");
				}
				if(QuSessMediaGetAttribute(media,"rtcp-fb","fir")==1){
					SessMediaAddAttribute(answer_media,"rtcp-fb",":* ccm fir");//a=rtcp-fb:109 ccm fir
				}
			}
			
			if(media_type==MEDIA_AUDIO) {
				if(QuSetIsQuantaProduct(-1, offer)){
					SessMediaAddAttribute(answer_media,"tool:",g_product);//a=tool:qsip v.0.0.1
				}
#if AUDIO_FEC
				printf("[AUDIO_FEC]payloadtype=%s\n",payloadtype);
				if(QuSetFec( payloadtype,offer )==QU_OK){
					pMediaPayload fec_payload=NULL;
					char rtpmap[32]="0";
					
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,"red/90000");
					SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                    
					memset(rtpmap,0,32*sizeof(char));
					snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
					//printf("rtpmap=%s\n",rtpmap);
					MediaPayloadNew(&fec_payload,AUDIO_FEC_PAYLOAD,rtpmap);
					SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                    
					//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
				}
#endif
			}else if(media_type==MEDIA_VIDEO) {
				//if (!strcmp(sip_config.generic_rtp.video_fec_enable,"1")){
                if (1){
					if(QuSetFec( payloadtype,offer )==QU_OK){
						pMediaPayload fec_payload=NULL;
						char rtpmap[32]="0";
						
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,"red/90000");
						SessMediaAddRTPParam(answer_media,RTPMAP,fec_payload);
                        
						memset(rtpmap,0,32*sizeof(char));
						snprintf(rtpmap,32*sizeof(char),"%s",payloadtype);
						//printf("rtpmap=%s\n",rtpmap);
						MediaPayloadNew(&fec_payload,VIDEO_FEC_PAYLOAD,rtpmap);
						SessMediaAddRTPParam(answer_media ,FMTP,fec_payload);
                        
						//printf("!!!!!!FEC!!!! payloadtype=%s\n",payloadtype);
					}
				}
			}

			
			/*set outband dtmf*/
			//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
			if (1){
				for(j=0;j<rtpmap_size;j++){
					if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
						payloadlen=255;
						payloadtypelen=31;
								
						if (MediaPayloadGetParameter(payload,payloadbuf,&payloadlen)==RC_OK){
							//printf("MediaPayloadGetParameter error\n");
						
							MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
							//char *slash=strchr(payloadbuf,'/');
							char payloadtmp[40]="0";
							strcpy (payloadtmp,payloadbuf);
							char *pch = strtok (payloadtmp,"/");
							//printf("%s\n",payloadtmp);
							int dtmf_type = atoi(payloadtype);
							//printf("%d\n",dtmf_type);
							if (!strcasecmp(pch,"telephone-event")){
								dtmf_type = atoi(payloadtype);
								//printf("!!!!!!!!\n%d\n",dtmf_type);
								pMediaPayload dtmf_payload=NULL;
								MediaPayloadNew(&dtmf_payload,payloadtype,"telephone-event/8000");
								SessMediaAddRTPParam(answer_media ,RTPMAP,dtmf_payload);
							}
						}		
					}
				}
			}
			
		}
		
		if (media_type==MEDIA_AUDIO && !is_audio_add_session ){
			//add new audio media
			SessAddMedia(qcall[id].lsdp,answer_media);
			is_audio_add_session = true;
		}else if (media_type==MEDIA_VIDEO && !is_video_add_session){
			//add new video media
			SessAddMedia(qcall[id].lsdp,answer_media);
			is_video_add_session = true;
		}else{
			printf("other media type or add audio/video media\n");
			pSessMedia new_media=NULL;
			SessMediaDup(&media,&new_media);
			SessMediaSetPort(new_media,0);
			SessAddMedia(qcall[id].lsdp,new_media);
		}
	}
	
	SessAddVersion(qcall[id].lsdp);
	
}

int QuSwitchCallMode(int mode ,char *addr){
	char sessbuf[SESS_BUFF_LEN]="0";
	UINT32 sess_len=SESS_BUFF_LEN-1 ;
	pCont	msgbody=NULL;
	pSessMedia audio_media=NULL;
	pSessMedia video_media=NULL;
	pMediaPayload payload=NULL;
	int media_size;
	int i;
	int id;
	id = QuGetCall(addr);
	printf("call id = %d\n",id);
	if (-1==id){
		return 0 ;
	}

	//generate SDP
	MediaPayloadNew(&payload,audio_codec[qcall[id].audio_codec].payload,audio_codec[qcall[id].audio_codec].codec);
	SessMediaNew(&audio_media,MEDIA_AUDIO,NULL,qcall[id].audio_port,"RTP/AVP",20,RTPMAP,payload);
	
	if (G7221 == qcall[id].audio_codec){
		//MediaPayloadNew(&payload,"121","bitrate=24000");
		
		char g7221_bitrate[32]="0";
		snprintf(g7221_bitrate,32*sizeof(char),"bitrate=%s",audio_codec[G7221].fmap);
		MediaPayloadNew(&payload,audio_codec[G7221].payload,g7221_bitrate);
		SessMediaAddRTPParam(audio_media ,FMTP,payload);
	}else if (OPUS == qcall[id].audio_codec){
		char opus_fmtp[256];
		snprintf(opus_fmtp,256*sizeof(char),"maxplaybackrate=%d;sprop-maxcapturerate=%d;maxaveragebitrate=%d;"
				"stereo=%d;useinbandfec=%d",16000,16000,24000,0,1);
		MediaPayloadNew(&payload,audio_codec[OPUS].payload,opus_fmtp);
		SessMediaAddRTPParam(audio_media ,FMTP,payload);

	}
	
	SessMediaAddAttribute(audio_media,"tool:",g_product);//a=tool:qsip v.0.0.1
	
	if(1){
	//if (!strcmp("2",sip_config.generic_lines.dtmf_transmit_method)){
		MediaPayloadNew(&payload,"101","telephone-event/8000");
		SessMediaAddRTPParam(audio_media ,RTPMAP,payload);
	}
	
	if (1==mode){
		local_video_port = qcall[id].video_port;
		GenVideoMedia(&video_media);
	}
	//clear old sdp
	SessGetMediaSize(qcall[id].lsdp,&media_size);
	for (i=0;i<media_size;i++){
		SessRemoveMedia(qcall[id].lsdp,0);
	}
	
	//add new audio media
	SessAddMedia(qcall[id].lsdp,audio_media);
	
	if (NULL!=video_media){//add new video media
		SessAddMedia(qcall[id].lsdp,video_media);
	}
	SessAddVersion(qcall[id].lsdp);
	
	if (SessToStr(qcall[id].lsdp,sessbuf,(INT32*)&sess_len)==RC_OK) //output string
			;//printf("****************\nsess sdp:\n%s\n",sessbuf);
	else printf("sess sdp fail\n");

	ccContNew(&msgbody,"application/sdp",sessbuf);
	
	//ccMakeCall(&newcall,"sip:7817689654@viperdevas01.walvoip.net",msgbody,newreq);
	ccModifyCall(&qcall[id].call_obj,msgbody);
	//ccMakeCall(&newcall,DialURL,msgbody,preq); //preq ccReqAddAuthz()
	ccContFree(&msgbody);
	
	/*2010-1012 added by nicky;record if switching medai mode now*/
	printf("qcall[id].request_mode=REQUEST_SWITCHMODE;\n");
	
	if (mode==AUDIO){
		qcall[id].request_mode=REQUEST_SWITCH_TO_AUDIO;
	}else if ((mode==VIDEO)){
		qcall[id].request_mode=REQUEST_SWITCH_TO_VIDEO;
	}
	//qcall[id].request_mode=REQUEST_SWITCHMODE;
	

	printf("End QuSwitchCallMode\n");
	return 1;
}

void QTSetIsQuantaProduct(int id){

	switch(qcall[id].isquanta)
	{
		case NONE_QUANTA:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_NA;
			break;
		case ET1:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_PX;
			break;
		case QT1A:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_QTA;
			break;
		case QT1W:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_QTW;
			break;
		case QT1I:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_QTI;
			break;
		case QUANTA_PX2:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_PX2;
			break;
		case QUANTA_MCU:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_MCU;
			break;
		case QUANTA_PTT:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_PTT;
			break;
		case QUANTA_OTHERS:
			qcall[id].qt_call->mCallData.product_type=call::PRODUCT_TYPE_OTHERS;
			break;
	}

	printf("***product_type = %d\n",qcall[id].qt_call->mCallData.product_type);
}

void *TransferNotifyExpireThread(void *arg){
    
	int timer = 0;
	int *tmp = (int*)arg;
	int id = *tmp;
	free(tmp);
    
	while (timer < 20 ){
		printf("TransferNotifyExpireThread  id=%d thrd=%d\n",id, qcall[id].thrd_transfer_notify_expire);
	    
		if (qcall[id].thrd_transfer_notify_expire)
			return NULL;
		
		usleep(500000);//timeout 10 sec;
		timer++;
	}
	
    if (qcall[id].transfer_mode == UNATTEND_TRANSFER){
        //timeout;
    }

	return NULL;

}

int QuRevReAck(SipReq req){
    
    pSess sess=NULL;
    pCont content=NULL;
    HoldStyle hs;
    
    char *displayname = GetReqFromDisplayname(req);
    if (!displayname){
        displayname=strdup("");
    }
    char phonenumber[64]="0";
    getPhoneFromReq(req,phonenumber);
    int id = QuGetCall(phonenumber);
    if (-1==id){
        return 0 ;
    }
    
    ccContNewFromReq(&content,req);
    
    UINT32 contlen=0;
    ccContGetLen(&content,&contlen);
    
    /*
     SipBody *body=NULL;
     body=sipReqGetBody(req);
     if (body!=NULL)
     printf("%s\n",body->content);
     */

    if (0 < contlen){
        
        char *contbuf = (char*) malloc (contlen+8);
        memset(contbuf,0, contlen+8);
        
        QuContToSessToStr(content,contbuf,&contlen);
        printf("sdp=%s\n",contbuf);
        
        if (NULL!=qcall[id].rsdp){
            SessFree(&(qcall[id].rsdp));
        }
        SessNewFromStr(&qcall[id].rsdp,contbuf);
        free(contbuf);
        
        SessCheckHold(qcall[id].rsdp,&hs);
        if (hs==CZEROS || AUDIO_SENDONLY==hs || AUDIO_RECVONLY==hs||AUDIO_INACTIVE==hs){
            printf("It's a hold sdp\n");
            printf("fw_inbound_hold :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
            
            if (HOLD != qcall[id].callstate ){
                qcall[id].callstate = BEHOLD;
                printf("Audio Mode:QuStreamHold:session id=%d\n",qcall[id].session);
            }
            
            //!----For QT----//
            
            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
            {
                // Enter the critical section
#if MUTEX_DEBUG
                printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                
                qcall[id].qt_call->mCallData.is_recv_hold = true;
                qcall[id].qt_call->mCallData.call_state = CALL_STATE_HOLD;
                qcall[id].qt_call->mCallData.is_reinvite = true;
                
                g_event_listner->onCallIncoming(qcall[id].qt_call);
                g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                printf("id=%d,qt=%d,state=%d\n",id,qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state);
                
#if MUTEX_DEBUG
                printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                //----------------------
            }
            
        }else{
            printf("QuRevReAck:Not Hold SDP\n");
            char remote_a_ip[32]="0";
            char remote_v_ip[32]="0";
            int laudio_port=0,lvideo_port=0;
            int raudio_port=0,rvideo_port=0;
            
            MediaCodecInfo media_info;
            MediaCodecInfoInit(&media_info);
            
            int callmode;
            
            QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
            QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
            QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
            QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
            QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
            
            if((0!=rvideo_port)&&(-1!=media_info.video_payload_type)){
                callmode=VIDEO;
            }else{
                callmode=AUDIO;
            }
            
            laudio_port = qcall[id].audio_port;
            lvideo_port = qcall[id].video_port;
            
            if (BEHOLD==qcall[id].callstate || HOLD==qcall[id].callstate){
                qcall[id].callstate = ACTIVE;
            }
            printf("fw_inbound_resume :displayname=%s,phonenumber=%s\n",displayname,phonenumber);
            
            /*reset audio and video codec*/
            QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
            QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
            
            QStreamPara media;
            set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
            
            //!!!!!!!It is for QT
            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
            {
                // Enter the critical section
#if MUTEX_DEBUG
                printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                
                qcall[id].qt_call->mCallData.is_reinvite = false;
                qcall[id].qt_call->mCallData.is_recv_hold = false;
                qcall[id].qt_call->mCallData.call_state = CALL_STATE_UNHOLD;
                setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
                g_event_listner->onCallIncoming(qcall[id].qt_call);
                g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
                
                /*
                  Check uhhold or transfer for Cisco
                */

                if (is_cisco_transfer == 1){
                	if (qcall[id].qt_call !=NULL){
						g_event_listner->onSessionEnd(qcall[id].qt_call);
					}
					g_event_listner->onSessionStart(qcall[id].qt_call);
                }
                is_cisco_transfer = 0;
                /*
                if (qcall[id].qt_call !=NULL){
                    g_event_listner->onSessionEnd(qcall[id].qt_call);
                }
                g_event_listner->onSessionStart(qcall[id].qt_call);
                */
                
#if MUTEX_DEBUG
                printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
            }
        }
    }
    
    return 1;
}


int QuRevAck(SipReq req){
    pSess sess=NULL;
    pCont content=NULL;
    HoldStyle hs;
    
    printf("QuRevAck\n");
    
    char *displayname = GetReqFromDisplayname(req);
    if (!displayname){
        displayname=strdup("");
    }
    char phonenumber[64]="0";
    getPhoneFromReq(req,phonenumber);
    int id = QuGetCall(phonenumber);
    
     printf("id=%d\n",id);
    
    if (-1==id){
        return 0 ;
    }
    
    qcall[id].is_rev_ack = true;
    ccContNewFromReq(&content,req);
    
    UINT32 contlen=0;
    ccContGetLen(&content,&contlen);
   
    
    if (0 < contlen){
        char *contbuf = (char*) malloc (contlen+8);
        memset(contbuf,0, contlen+8);

        QuContToSessToStr(content,contbuf,&contlen);
        printf("sdp=%s\n",contbuf);
        if (NULL!=qcall[id].rsdp){
            SessFree(&(qcall[id].rsdp));
        }
        SessNewFromStr(&qcall[id].rsdp,contbuf);
        free(contbuf);
        
        char remote_a_ip[32]="0";
        char remote_v_ip[32]="0";
        int laudio_port=0,lvideo_port=0;
        int raudio_port=0,rvideo_port=0;
        
        MediaCodecInfo media_info;
        MediaCodecInfoInit(&media_info);
        
        QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
        QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
        QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
        QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
        QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
        
        
        if((0!=rvideo_port)&&(-1!=media_info.video_payload_type)){
            qcall[id].media_type=VIDEO;
        }else{
            qcall[id].media_type=AUDIO;
        }
        
        laudio_port = qcall[id].audio_port;
        lvideo_port = qcall[id].video_port;
        
        QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
        QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
        
        /* identy is Quanta product*/
        //PreAnswer had set isquanta
        if (qcall[id].isquanta != QUANTA_PTT){
        	QuSetIsQuantaProduct(id, qcall[id].rsdp);
        }
        
        QTSetIsQuantaProduct(id);
        printf("id=%d, phone=%s\n",id,qcall[id].phone);
        
        QStreamPara media;
        
        set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
        QuQstreamPrintParameter(&media);
        
        setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
    
        /*set qt_call information*/
        if(g_event_listner != NULL && qcall[id].qt_call != NULL)
        {
            // Enter the critical section
    #if MUTEX_DEBUG
            printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
    #endif
            pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
            
            char audio_mime[16];
            char video_mime[16];
            memset(audio_mime, 0, sizeof(audio_mime));
            memset(video_mime, 0, sizeof(video_mime));
            strcpy(audio_mime, qcall[id].qt_call->mCallData.audio_mdp.mime_type);
            strcpy(video_mime, qcall[id].qt_call->mCallData.video_mdp.mime_type);
            
            
            qcall[id].qt_call->mCallData.is_session_start = true;
            printf("onSessionStart\n");
            
            if (qcall[id].qt_call !=NULL){
                g_event_listner->onSessionEnd(qcall[id].qt_call);
            }
            g_event_listner->onSessionStart(qcall[id].qt_call);
            
            // Enter the critical section
    #if MUTEX_DEBUG
            printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
    #endif
            pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
            
        }
    
    }
    
    ccContFree(&content);
    
    return 1;
}


void QuStartTransferNotifyExpireThread(char *addr){
	
	int id=QuGetCall(addr);
	printf("QuStartTransferNotifyExpireThread %s,%d\n",addr,id);
	
	qcall[id].thrd_transfer_notify_expire = false;
	
	
	int *p_id = (int*) malloc(sizeof(int));
	*p_id = id;
	
	pthread_t transferNotifyExpire;
	pthread_create(&transferNotifyExpire, NULL, TransferNotifyExpireThread, (void*) p_id);
	pthread_detach(transferNotifyExpire);
}

int QuGetCallFromICall(itricall::ItriCall* pcall){
    int i=0;
    for (i=0;i<NUMOFCALL;i++){
        if (qcall[i].qt_call==pcall)
            return i;
    }
    
    printf("[error] can't find qt internal call\n");
    return -1;
}

int QuSetFec( char*match_payloadtype, pSess sess ){
	int media_size;
	pSessMedia media=NULL;
	MediaType media_type;
	pMediaPayload payload=NULL;
	pMediaPayload fmtp_payload=NULL;
	
	char payloadbuf[256]="0";
	int payloadlen=255;
	char payloadtype[32]="0";
	int payloadtypelen;
	UINT32 rtpmap_size;
	int i,j;
    
	SessGetMediaSize(sess,&media_size);
	for ( i=0 ; i< media_size ; i++ ){
		SessGetMedia(sess,i,&media);
		SessMediaGetType(media,&media_type);
		SessMediaGetRTPParamSize(media,RTPMAP,&rtpmap_size);
		//if (media_type==MEDIA_VIDEO){
		//printf("QuSetFec i=%d,media_size=%d\n",i,media_size);
		//printf("QuSetFec i=%d,media_type=%d\n",i,media_type);
		
		if (1){
			for(j=0;j<rtpmap_size;j++){
				
				if(SessMediaGetRTPParamAt(media,RTPMAP,j,&payload)==RC_OK){
					
					payloadtypelen=31;
					MediaPayloadGetType(payload,payloadtype,&payloadtypelen);
					payloadlen=255;
					MediaPayloadGetParameter(payload,payloadbuf,&payloadlen);
					char payloadtmp[40]="0";
					strcpy (payloadtmp,payloadbuf);
					char *pch = strtok (payloadtmp,"/");
					printf("QuSetFec payload=%s\n",pch);
					if (!strcasecmp(pch,"red")){
						printf("red , payloadtype=%s\n",payloadtype);
						SessMediaGetRTPParam(media,FMTP,payloadtype,&fmtp_payload);
						
						payloadlen=255;
						MediaPayloadGetParameter(fmtp_payload,payloadbuf,&payloadlen);
						char payloadtmp[128]="0";
						strcpy (payloadtmp,payloadbuf);
                        
						char *fmtp_pch = strtok(payloadtmp,"/ ");
						printf("QuSetFec fmtp_pch=%s\n",fmtp_pch);
						while(fmtp_pch != NULL)
						{
							if(!strcmp(fmtp_pch,match_payloadtype)){
								return 1;
							}
							fmtp_pch=strtok(NULL,"/");
							printf("QuSetFec fmtp_pch=%s\n",fmtp_pch);
						}
						
						//return -1;
					}
				}
				
			}
		}
		
	}
    
	return -1;
}

int GetSipFrag(pCont content) {
	if(!content)
		return -1;
	
	char buf[1024];
	UINT32 buflen=1024;
	memset(buf,0,sizeof(buf));
	
	if(RC_OK!=ccContGetType(&content,buf,&buflen))
		return -1;
	
	printf("sip_frag=%s\n",buf);
	
	if(strstr(buf,"message/sipfrag")==NULL)
		return -1;
	
	buflen = 1024;
	memset(buf,0,sizeof(buf));
    
	if(RC_OK!=ccContGetBody(&content,buf,&buflen))
		return -1;
	
	printf("sip_frag_body=%s\n",buf);
	char* token = strchr( buf , ' ' );
	
	if(!token)
		return -1;
    
	token++;
	char* token1 = strchr( token , ' ' );
	
	if(!token1)
		return -1;
	else
		*token1='\0';
    
	return atoi(token);
}

void *QuAddConferenceParticipants(void *arg){
    
    int *tmp = (int*)arg;
	int call_id = *tmp;
	free(tmp);

    sleep(1);

    char pre_phone[64]="0";

    //g_event_listner->onSessionEnd(qcall[call_id-1].qt_call);

    if (QuCheckInSession()){
        printf("[SIP]QuAddConferenceParticipants call self =%s, phone=%s\n", qcall[call_id].phone ,qcall[1-call_id].phone);
        /*record previous phone number*/
        strcpy(pre_phone, qcall[1-call_id].phone);
        //if (!strcmp(qcall[call_id-1].phone, qcall[call_id].phone)){
            sip_unattend_transfer(qcall[call_id].phone, qcall[1-call_id].phone);
        //}
        sleep(1);
    }
    printf("[SIP]QuAddConferenceParticipants , call_id=%d\n",call_id);
    int i=0;
    for (i=0;i<3;i++){
        if (qcall[call_id].cp_phone[i] !=NULL){
            printf("[SIP]sip_unattend_transfer ,phone=%s,target=%s\n",qcall[call_id].phone,qcall[call_id].cp_phone[i]);

            if (strcmp(pre_phone, qcall[call_id].cp_phone[i])){
                sip_unattend_transfer(qcall[call_id].phone, qcall[call_id].cp_phone[i]);
                sleep(1);
            }

        }
    }


    return NULL;
}
