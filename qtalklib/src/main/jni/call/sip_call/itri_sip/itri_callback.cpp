#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include   <windows.h>
	#include "DbgUtility.h"
	#define printf dxprintf
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
#include <assert.h>
#define ITRI_CALL_STATE_RINGING 2

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
call::ICallEventListener* g_event_listner;

Config sip_config;
QuRegister reg;
QuProfile profile;
RCODE rCode=RC_OK;
pthread_t threadGeteXEvents=NULL;

bool g_inviteProgress =FALSE;
bool thrd_reg = true;

QuRegister se;

QuCall qcall[NUMOFCALL];
char outbound_url[256]="sip:0.0.0.0:5060";

int media_bandwidth=4096;

extern int local_audio_port;
extern int local_video_port;
int g_local_listen_port = LOCALPORT;
int enable_srtp = 0;

extern bool thrd_event;

QuAudioCodec audio_codec[NUM_OF_AUDIO_CODEC];

bool thrd_event = TRUE;

bool thrd_register_retry = true;
pthread_t threadExpire;
pthread_t threadRegisterRetry=-1;

bool g_prack_enable = false;

char g_product[80];

int g_network_inetrface_type=0;
bool g_isMCUConference=FALSE; // MCU solution

QTState qt_state;

/*avoid logout and incoimd at the same time*/
extern bool b_itri_init;
extern pthread_mutex_t qu_callengine_mutex;
extern pthread_mutex_t qu_free_mutex;

int g_register_progress = FALSE;
struct call::MDP    local_video_mdp;

int QuRegisterFail(const char* msg);

void set_profile(char* proxysv, char* outboundsvr, char* realm, char* sipid ,char* name, char* psw , char* local_ip){

}
void set_event_listner( call::ICallEventListener* listener){
	g_event_listner = listener;
}

void set_prack_support( bool prack_enable){
	g_prack_enable = prack_enable;
}


SipReq newReqFromOldReq(pCCCall call,SipReq oldreq,const char* newurl,pSipDlg* pcallleg)
{
	//new request
	
	if(!oldreq || !call || !pcallleg)
		return NULL;

	SipReq retreq=sipReqDup(oldreq);
	SipMethodType method=GetReqMethod(retreq);

	pSipDlg callleg=*pcallleg;
	SipReq newreq=NULL;
	if(!callleg) {
		ccNewReqFromCall(&call,&newreq,method,newurl,NULL);
	} else{
		DlgState dstate=DLG_STATE_NULL;
		sipDlgGetDlgState(&callleg,&dstate);
		//if callleg is not confirm,input newurl for created new callleg
		if(dstate==DLG_STATE_CONFIRM){
			ccNewReqFromCallLeg(&call,&newreq,method,NULL,NULL,&callleg);
		} else{
			pSipDlg newcallleg=NULL;
			ccNewReqFromCallLeg(&call,&newreq,method,newurl,NULL,&newcallleg);
			*pcallleg=newcallleg;
		}		
	}
	
	//update request URI
	sipReqSetReqLine(retreq,sipReqGetReqLine(newreq));
	
	//update new header to old request
	sipReqAddHdr(retreq,Call_ID,sipReqGetHdr(newreq,Call_ID));
	sipReqAddHdr(retreq,CSeq,sipReqGetHdr(newreq,CSeq));
	//sipReqAddHdr(retreq,From,sipReqGetHdr(newreq,From));
	sipReqAddHdr(retreq,To,sipReqGetHdr(newreq,To));
	
	sipReqDelHdr(retreq,Authorization);
	/*added by nicky 2010-11-24*/
	/*remove Proxy-Authorization*/
	sipReqDelHdr(retreq,Proxy_Authorization);
	
	//release new req
	sipReqFree(newreq);
	newreq = NULL;

	//remove old via
	sipReqDelHdr(retreq,Via);

	return retreq;
}


int sip_make_registration(const char* proxysv,const char* outboundsvr,const char* realm,const char* sipid
		,const char* name,const char* psw, bool isLogin)
{
	   
	/*register to Proxy Server*/
	SipReq	req=NULL;
	int buffer_size = 256;
	char rurl[buffer_size];
	char usercontact[buffer_size];
	char outbound[buffer_size];
	
	if (reg.regcall!=NULL){
		ccCallFree(&reg.regcall);
		reg.regcall=NULL;
	}

	if (reg.user!=NULL){
		ccUserFree(&reg.user);
		reg.user=NULL;
	}

	if (reg.regreq){
		sipReqFree(reg.regreq);
		reg.regreq=NULL;
	}

	if (reg.regrsp){
		sipRspFree(reg.regrsp);
		reg.regrsp=NULL;
	}

	printf ("sip_make_registration\n");	

	strncpy(profile.proxy_server, proxysv, 128*sizeof(char));	//"viperdevas01.walvoip.net";
	strncpy(profile.outbound_server, outboundsvr, 128*sizeof(char));	// "sip.vzalpha.com";
	strncpy(profile.realm, realm, 128*sizeof(char)); //"viperdevas01.walvoip.net";
	strncpy(profile.sipid, sipid, 64*sizeof(char)); // 7817689678 || x1020
	strncpy(profile.username, name, 64*sizeof(char)); // 7817689678 || 1020
	strncpy(profile.password, psw, 64*sizeof(char)); //quanta
	
	printf("%s,%s,%s,%s,%s,%s\n",proxysv,outboundsvr,realm,sipid,name,psw);
	memset(rurl,0, buffer_size*sizeof(char));
	
	snprintf(rurl, buffer_size*sizeof(char),"sip:%s",profile.proxy_server);
	
	memset(usercontact,0,buffer_size*sizeof(char));
	if (!strcmp("1",sip_config.generic.sip.tls_enable)){
		snprintf(usercontact, buffer_size*sizeof(char),"<sip:%s@%s:%d;transport=tls>",profile.sipid,profile.ip,g_local_listen_port+1);
	}else{
		snprintf(usercontact, buffer_size*sizeof(char),"<sip:%s@%s:%d>",profile.sipid,profile.ip,g_local_listen_port);
		//sprintf(usercontact,"<sip:%s@%s:%d;transport=tcp>",profile.sipid,profile.ip,g_local_listen_port);
	}

	printf("usercontact=%s",usercontact);
	memset(outbound,0, buffer_size*sizeof(char));
	snprintf(outbound, buffer_size*sizeof(char),"<sip:%s:%d;lr>",profile.outbound_server,5060);
	
	memset(outbound_url,0,sizeof(outbound_url));
	if (!strcmp("1",sip_config.generic.sip.tls_enable)){
		snprintf(outbound_url, buffer_size*sizeof(char),"sips:%s:%d",profile.outbound_server,5061);
	}else{
		snprintf(outbound_url, buffer_size*sizeof(char),"sip:%s:%d",profile.outbound_server,5060);
		//sprintf(outbound_url,"sip:%s:%d;transport=tcp",profile.outbound_server,5060);
	}


	ccCallNew(&reg.regcall);
	if(ccUserNew(&reg.user,profile.sipid)==RC_OK)
		;//printf("new user is done\n");

	if(ccUserSetAddr(&reg.user,profile.proxy_server)==RC_OK)
		;//printf("set user addr is done\n");

	if (ccUserSetCredential(&reg.user,"realm",profile.username,profile.password)==RC_OK) {		
		;//printf("SetCredential is done\n");
	}
	
	if(ccUserSetDisplayname(&reg.user,profile.sipid)==RC_OK)
		;//printf("set display name is done\n");
	
	//ccUserSetDisplayname(&reg[regid].user,"anonymous");
	
	if(ccUserSetContact(&reg.user,usercontact)==RC_OK)
		;//printf("set contact is done\n");

	if(ccCallSetUserInfo(&reg.regcall,reg.user)==RC_OK)
		;//printf("set user to call is done!\n");

	/*
	if (ccUserFree(&user)==RC_OK) 
		printf("free user!\n");
	*/
	
	if(ccNewReqFromCall(&reg.regcall,&req,REGISTER,rurl,NULL)==RC_OK){
		printf("set new request from call ok!\n");
		
	}

	ccReqAddHeader(req,Contact,usercontact);
	/*set register Expire time , isLogin is false , set expire time 0*/
	if(isLogin){
		ccReqAddHeader(req,Expires,REGISTER_EXPIRE);
	}else{
		ccReqAddHeader(req,Expires,"0");
	}

	printf("outbound_url=%s\n",outbound_url);
	if (ccCallSetOutBound(&reg.regcall,outbound_url,profile.ip,g_local_listen_port) ==RC_OK)
	{
		printf ("set outbound proxy ok!\n");
	}	
	
	printf("\n%s\n",sipReqPrint(req));
	
    //sipReqSetUseShortForm(req,FALSE);
    
	if (ccSendReq(&reg.regcall,req,NULL) ==RC_OK){
		reg.handle_cseq = 0;
		printf ("Send regist request ok!\n");
	} else
    {
        printf ("Send regist request NOT ok!!!!!!!!!!!!!!!!!\n");
    }

	if(req)
		sipReqFree(req);

	return RC_OK;
}

void *logout(void *arg){
    
    sleep(1);
    g_inviteProgress=FALSE;
    
    if (reg.startkplive) {
        thrd_reg = false;
        pthread_join(reg.threadRegister,NULL);
        reg.startkplive=false;
    }
    
    if (reg.regcall!=NULL){
        ccCallFree(&reg.regcall);
        reg.regcall=NULL;
    }
    
    if (reg.user!=NULL){
        ccUserFree(&reg.user);
        reg.user=NULL;
    }
    
    if (reg.regreq){
        sipReqFree(reg.regreq);
        reg.regreq=NULL;
    }
    
    if (reg.regrsp){
        sipRspFree(reg.regrsp);
        reg.regrsp=NULL;
    }
    
    SipReq	req=NULL;
	int buffer_size = 256;
	char rurl[buffer_size];
	char usercontact[buffer_size];
	char outbound[buffer_size];
	
	printf ("sip_logout\n");
    
    memset(rurl,0,buffer_size*sizeof(char));
	snprintf(rurl, buffer_size*sizeof(char),"sip:%s",profile.proxy_server);
	
	memset(usercontact,0, buffer_size*sizeof(char));
	snprintf(usercontact, buffer_size*sizeof(char),"<sip:%s@%s:5060>",profile.sipid,profile.ip);
    
    memset(outbound,0, buffer_size*sizeof(char));
	snprintf(outbound, buffer_size*sizeof(char),"<sip:%s:%d;lr>",profile.outbound_server,5060);
	
	memset(outbound_url,0, buffer_size*sizeof(char));
	snprintf(outbound_url, buffer_size*sizeof(char),"sip:%s:%d",profile.outbound_server,5060);
    
    ccCallNew(&reg.regcall);
	if(ccUserNew(&reg.user,profile.sipid)==RC_OK)
		;//printf("new user is done\n");
    
	if(ccUserSetAddr(&reg.user,profile.proxy_server)==RC_OK)
		;//printf("set user addr is done\n");
    
	if (ccUserSetCredential(&reg.user,"realm",profile.username,profile.password)==RC_OK) {
		;//printf("SetCredential is done\n");
	}
	
	if(ccUserSetDisplayname(&reg.user,profile.sipid)==RC_OK)
		;//printf("set display name is done\n");
	
	//ccUserSetDisplayname(&reg[regid].user,"anonymous");
	
	if(ccUserSetContact(&reg.user,usercontact)==RC_OK)
		;//printf("set contact is done\n");
    
	if(ccCallSetUserInfo(&reg.regcall,reg.user)==RC_OK)
		;//printf("set user to call is done!\n");
    
    if(ccNewReqFromCall(&reg.regcall,&req,REGISTER,rurl,NULL)==RC_OK){
		printf("set new request from call ok!\n");
		
	}
    
    ccReqAddHeader(req,Contact,usercontact);
    //set logout Expire 0
    ccReqAddHeader(req,Expires,"0");
    
    if (ccSendReq(&reg.regcall,req,NULL) ==RC_OK){
		printf ("Send regist request ok!\n");
	}
    
	if(req)
		sipReqFree(req);
    
    return NULL;
}

void *kpliveThread(void *arg){
	
	QuRegister kp_se;
	
	memcpy(&kp_se,arg,sizeof(QuRegister));
	printf("kp_se=%d\n",kp_se.expires);
	SipReq newreq=NULL;
	//printf("expires = %d\n",se->expires);
	int buffer_size = 256;
	char contact[buffer_size];
	char rount[buffer_size];
	
	memset(contact , 0, buffer_size*sizeof(char));
	memset(rount , 0, buffer_size*sizeof(char));
	
	snprintf(contact,buffer_size*sizeof(char),"<sip:%s@%s>",profile.sipid,profile.ip);
	snprintf(rount,buffer_size*sizeof(char),"<sip:%s:%d;lr>",profile.outbound_server,LOCALPORT);

	/**/
	char buf[32]="0";
	thrd_reg = true;
	int reg_count=0;
	int expire_time;

	expire_time =  (kp_se.expires)*0.9;

	printf("register expire time = %d ,%d\n",se.expires,expire_time);
	if (se.expires<=0){
		thrd_reg=false;
	}
	
	while(thrd_reg){
		
		reg_count++;

		if (reg_count > expire_time){
			//printf("expires = %d\n",se.expires);
			
			if(ccNewReqFromCall(&kp_se.regcall,&newreq,REGISTER,GetReqUrl(kp_se.regreq), NULL)!=RC_OK){
				printf("ccNewReqFromCall not OK\n");
			}
			
			//sleep((se.expires)*0.9);
			sipReqAddHdr(se.regreq,CSeq,sipReqGetHdr(newreq,CSeq));
			
			//*2010-08-20 remove old via*/
			sipReqDelHdr(se.regreq,Via);
			
			if (ccCallSetOutBound(&kp_se.regcall,outbound_url,profile.ip,g_local_listen_port) !=RC_OK){
				printf ("keep alive set outbound proxy not ok!=%s\n",outbound_url);
			}
			
            
			if (thrd_reg){
				if (RC_OK!=ccSendReq(&kp_se.regcall,se.regreq,NULL))
					printf("Send REGISTER fail!\n");
			}

			UINT32 buflen=31;
			ccUserGetUserName(&kp_se.user,buf,&buflen);
			//printf("expires = %d user=%s\n",se->expires,buf);
			
			if(newreq)
				sipReqFree(newreq);
			reg_count=0;
		}
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
	/*
    if(ccNewReqFromCall(&kp_se.regcall,&newreq,REGISTER,GetReqUrl(kp_se.regreq), NULL)==RC_OK){
        printf("ccNewReqFromCall OK\n");
    }
    
    //sleep((se.expires)*0.9);
    sipReqAddHdr(se.regreq,CSeq,sipReqGetHdr(newreq,CSeq));
    sipReqDelHdr(se.regreq,Via);
    
    ccReqAddHeader(se.regreq,Expires,"0");
   
    
     if (ccCallSetOutBound(&kp_se.regcall,outbound_url,profile.ip,LOCALPORT) !=RC_OK){
     printf ("keep alive set outbound proxy not ok!=%s\n",outbound_url);
     }
    
    
    if (RC_OK!=ccSendReq(&kp_se.regcall,se.regreq,NULL))
        printf("Send REGISTER fail!\n");

    if(newreq)
        sipReqFree(newreq);
    */
    
	printf("pthread_exit\n");
	pthread_exit(0);

	return NULL;
}

void *start_register_retry(void *arg){

	int res;

	int retry_interval=atoi(sip_config.generic.lines.registration_resend_time);

	printf("retry interval time= %d\n",retry_interval);

	thrd_register_retry=true;
	int count=0;

	while(thrd_register_retry){

		if (count>retry_interval){
			if (reg.startkplive) {
				thrd_reg = false;
				if (reg.threadRegister!=-1){
					pthread_join(reg.threadRegister,NULL);
				}
				reg.threadRegister = -1;
				reg.startkplive = false;
			}

			if (reg.regcall!=NULL){
				ccCallFree(&reg.regcall);
				reg.regcall=NULL;
			}

			if (reg.user!=NULL){
				ccUserFree(&reg.user);
				reg.user=NULL;
			}

			if (reg.regreq){
				sipReqFree(reg.regreq);
				reg.regreq=NULL;
			}

			if (reg.regrsp){
				sipRspFree(reg.regrsp);
				reg.regrsp=NULL;
			}

			res=0;

			sip_make_registration(profile.proxy_server, profile.outbound_server, profile.realm,
							profile.sipid, profile.username, profile.password, true);

			thrd_register_retry = false;
		}

		sleep(1);
		count++;
	}

	printf("threadRegisterRetry Exit\n");
	threadRegisterRetry = -1;
	return NULL;
}

int sip_make_call(itricall::ItriCall* call, unsigned int mode, char* ruuid, char* addr, int audio_port , int video_port)
{
//void sip_make_call(unsigned int mode, char* addr, int audio_port , int video_port){


	int freeId;
	int buffer_size = 256;
	char ourl[buffer_size];
	char rurl[buffer_size];
	pMediaPayload payload=NULL;
	pSessMedia audio_media=NULL;
	pSessMedia video_media=NULL;
	pSess sess=NULL;
	char sessbuf[SESS_BUFF_LEN]="0";
	INT32 sesslen = SESS_BUFF_LEN-1;
	pCont	msgbody=NULL;
	int register_code;
	
	if (mode< 0 || mode > 1 || addr==NULL || *addr==0){
		printf("sip_make_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	printf("sip_make_call=>mode=%d , ruuid=%s, addr=%s\n", mode, ruuid, addr);

	int ack_value;
	//2012/03/09 check if phonenumber is equal the number in the qcall_call talbe
	int i;

	if (g_inviteProgress){
		printf("g_inviteProgress is true\n");
		return RESULT_INVITE_PROGRESS;
	}

	for(i=0;i<NUMOFCALL;i++){
		if (!strcmp(qcall[i].phone,addr)){
            printf("!!!the number is in the qcall table\n");
			return RESULT_INVITE_INSESSION;
		}
	}
	
	register_code = reg.is_register;
	if (register_code > 300 || register_code == -1){
		printf("register_code=%d\n",register_code);
		if (register_code != 401 && register_code !=407){
			return RC_ERROR;
		}
	}


	SipReq req=sipReqNew();
	
	freeId=QuGetFreeCall();
	
	if (freeId == -1){
		g_inviteProgress=false;
		printf("QuGetFreeCall Fail! %d\n",freeId);
		return RC_ERROR;
	}
	
	printf("free call ID =%d , laudio_port=%d,lvideo_port=%d\n",freeId,audio_port,video_port);
	
	qcall[freeId].qt_call=call;
	//local_audio_port +2
	local_audio_port = audio_port;
	//generate SDP	
	
	GenAudioMedia(&audio_media);
    
	if (1==mode){
		local_video_port = video_port;
		GenVideoMedia(&video_media);
		
	}
	
	qcall[freeId].audio_port=local_audio_port;
	qcall[freeId].video_port=local_video_port;
	
	SessNew(&sess,profile.ip,audio_media,0);//bandwidth 2048kbps
	SessAddBandwidth(sess,"CT",media_bandwidth);
	
	if(NULL!=video_media){	
		SessAddMedia(sess,video_media);
	}
	
	/*rfc3108 2010-0820*/
	if (RC_OK==SessSetStatus(sess,SENDRECV)){
		//printf("SessSetStatus SENDONLY OK!!!\n");
	}
	
	if (SessToStr(sess,sessbuf,&sesslen)==RC_OK) //output string
		SessFree(&sess);//printf("sess sdp:\n%s\n",sessbuf);
	
	printf("sess sdp:\n%s\n",sessbuf);

	ccContNew(&msgbody,"application/sdp",sessbuf);

	ccCallNew(&qcall[freeId].call_obj);
	
	//memcpy(&qcall[freeId].lsdp , &sess , sizeof(qcall[freeId].lsdp));
	//qcall[freeId].lsdp = sess;
	SessNewFromStr(&qcall[freeId].lsdp,sessbuf);

	strcpy(qcall[freeId].phone, addr);
		
	qcall[freeId].media_type = (int)mode;

	printf("session_id = %d\n",qcall[freeId].session);
	
	memset(ourl,0,buffer_size*sizeof(char));
	snprintf(ourl,buffer_size*sizeof(char),"<sip:%s:5060;lr>",profile.outbound_server);	
	memset(rurl,0,buffer_size*sizeof(char));
	//modified by nicky;repalace # with %23
	

	char addr_tmp[buffer_size];
	strncpy(addr_tmp,str_replace(qcall[freeId].phone,(char *)"#",(char *)"%23"),buffer_size*sizeof(char)); 
	
	char sip_address_mapping_mechanism[8]="3";

	if (!strncmp("1",sip_address_mapping_mechanism, 8*sizeof(char))){
		const char *sub_at_str=NULL;
		sub_at_str=strstr(addr_tmp,"@");
		if (sub_at_str==NULL){
			
			if (addr_tmp[0]=='+'){
				snprintf(rurl,buffer_size*sizeof(char),"sip:%s@%s;user=phone",addr_tmp,profile.proxy_server );
			}else{
				snprintf(rurl,buffer_size*sizeof(char),"sip:%s@%s",addr_tmp,profile.proxy_server );
			}
		}else{
			snprintf(rurl,buffer_size*sizeof(char),"sip:%s",addr_tmp);
		}
		
	}else if (!strcmp("2",sip_address_mapping_mechanism)){
		//set scheme use <form>tel:
		//if (sip_config.device_sip.e_164_phone_number[0]=='+'){
		{
			//ccUserSetUserName(&reg.user,sip_config.device_sip.e_164_phone_number);
			ccUserSetScheme(&reg.user,"tel");
		}
		snprintf(rurl,buffer_size*sizeof(char),"tel:%s",addr_tmp);
	}else if (!strncmp("3",sip_address_mapping_mechanism, 8*sizeof(char))){
		
		ccUserSetUserName(&reg.user,profile.sipid);
		ccUserSetScheme(&reg.user,"sip");
	
		const char *sub_at_str=NULL;
		sub_at_str=strstr(addr_tmp,"@");
		if (sub_at_str==NULL){
			if (addr_tmp[0]=='+'){
				//set scheme use <from>tel:
				//if (sip_config.device_sip.e_164_phone_number[0]=='+'){
				{
					//ccUserSetUserName(&reg[0].user,sip_config.device_sip.e_164_phone_number);
					ccUserSetScheme(&reg.user,"tel");
				}
				snprintf(rurl,buffer_size*sizeof(char),"tel:%s",addr_tmp);
			}else{
				//sprintf(rurl,"tel:%s;phone-context=%s",addr_tmp,profile.proxy_server);//support local number
				snprintf(rurl,buffer_size*sizeof(char),"sip:%s@%s",addr_tmp,profile.proxy_server );
			}
		}else{
			if (addr_tmp[0]=='+'){
				snprintf(rurl,buffer_size*sizeof(char),"sip:%s;user=phone",addr_tmp);
			}else{
				snprintf(rurl,buffer_size*sizeof(char),"sip:%s",addr_tmp);
			}	
			
		}
	}else{
		if (addr_tmp[0]=='+'){
			snprintf(rurl,buffer_size*sizeof(char),"sip:%s@%s;user=phone",addr_tmp,profile.proxy_server );
		}else{
			snprintf(rurl,buffer_size*sizeof(char),"sip:%s@%s",addr_tmp,profile.proxy_server );
		}
	}	
	
	//ccUserNew(&user,USERNAME);
	//ccUserSetContact(&user,"(@@)<sip:test123@127.0.0.1>");
	//ccUserSetUserName(&reg[0].user,"anonymous");
	ccCallSetUserInfo(&qcall[freeId].call_obj,reg.user);
	//ccUserFree(&user);
	
	ccReqAddHeader(req,Max_Forwards,"70");//rfc 3261
	ccReqAddHeader(req,Expires,EXPIRES);//expires time 60s
	//ccReqAddHeader(req,User_Agent,g_user_agent);
	
	memset(qcall[freeId].sip_sendip,0,32*sizeof(char));
	
	ccCallSetOutBound(&qcall[freeId].call_obj,outbound_url,profile.ip,g_local_listen_port);
	
    /*set Remote display name*/
	if (NULL != ruuid) {
		ccSetRemoteDisplayName(ruuid);
	}

	if (strcmp("sip:0.0.0.0:5060",qcall[freeId].sip_sendip)){
		ccMakeCall(&qcall[freeId].call_obj,rurl,msgbody,req);
	}
	
	//2012/03/12 added by nicky , record callid
	char callid[128];
	UINT32 callid_len=127;
	ccCallGetCallID(&qcall[freeId].call_obj,callid,&callid_len);
	qcall[freeId].callid=strdup(callid);
	
	sipReqFree(req);
	ccContFree(&msgbody);
	printf("sip_make_call : mode=%d\n",mode);	
	//printf("mode = %d, addr = %s\n" , mode, addr);
    
    /*save sip CalldID and iscaller in icall struct for video archive*/
    qcall[freeId].qt_call->mCallData.is_video_arch_caller = true;
    printf("**** is_caller=%d\n",qcall[freeId].qt_call->mCallData.is_video_arch_caller);
    strncpy (qcall[freeId].qt_call->mCallData.call_id,callid, 128*sizeof(char));
    strncpy (qcall[freeId].qt_call->mCallData.local_id,profile.sipid, 128*sizeof(char));
    
    //QuStartCallerExpireThread(addr); //disable caller invite timer
    
	return freeId;

}


int sip_answer_call(itricall::ItriCall* call,int mode, char* addr)
{
	int id;
	
	int sesslen=SESS_BUFF_LEN-1;
	char sessbuf[SESS_BUFF_LEN]="0";
	pCont	msgbody=NULL;
	pSessMedia audio_media=NULL;
	pSessMedia video_media=NULL;
	pSess sess=NULL;
	char remote_a_ip[32]="0";
	char remote_v_ip[32]="0";
	int raudio_port=0,rvideo_port=0;
	
	MediaCodecInfo media_info;
	MediaCodecInfoInit(&media_info);

	if (mode <0 || mode >1 || addr==NULL ||*addr==0 ){
		printf("sip_answer_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	id = QuGetCall(addr);
	
	if (-1==id){
		printf("[Error]:sip_answer_call:callid -1\n");
		return RC_ERROR;
	}
	
	if (EMPTY!=qcall[id].callstate){
		printf("[Error]:sip_answer_call:callstate is not empty\n");
		return RC_ERROR;
	}
	
	printf("sip_answer_call:addr=%s, id = %d\n",addr,id);
	
	qcall[id].qt_call=call;
	QTSetIsQuantaProduct(id);

	if (qcall[id].rsdp!=NULL){
		QuOfferToAnswerToStr(id,qcall[id].rsdp,sessbuf,&sesslen);
		
		SessNewFromStr(&qcall[id].lsdp,sessbuf); 
		
		printf("answer mode=%d\n",mode);
		
		/*add 01-29*/
		if (0==mode){
			int i;
			int media_size;
			pSessMedia media=NULL;
			MediaType media_type;
			
			qcall[id].media_type=AUDIO;
		
			char zero_video_port[2]="1";
			//GetStrDataFromFile(DEFAULT_CONFIG_NAME, "ZERO_VIDEO_PORT", zero_video_port);
			
			if (!strcmp("1",zero_video_port)) {
			    /* 2010-04-19  don't set video port=0 */
				    
			    SessGetMediaSize(qcall[id].lsdp,&media_size);
				    
			    for (i=0;i<media_size;i++){
				    SessGetMedia(qcall[id].lsdp,i,&media);
				    SessMediaGetType(media,&media_type);
				    if (media_type==MEDIA_VIDEO){
					    if (RC_OK==SessMediaSetPort(media,0))
						    printf("change Video Port 0\n");
					    else printf("ERROR\n");
				    }
			    }
			} else {
				    
			    SessGetMediaSize(qcall[id].lsdp,&media_size);
				    
			    for (i=media_size-1;i>=0;i--){
				    SessGetMedia(qcall[id].lsdp,i,&media);
				    SessMediaGetType(media,&media_type);
					    
				    if (media_type==MEDIA_VIDEO){
					    SessRemoveMedia(qcall[id].lsdp,i);
				    }
			    }	
			}

		}else {
			qcall[id].media_type=VIDEO;
		}
		
		sesslen=SESS_BUFF_LEN-1;
		SessToStr(qcall[id].lsdp,sessbuf,&sesslen);
		
		//printf("Answer 200OK SDP:\n%s",sessbuf);
		ccContNew(&msgbody,"application/sdp",sessbuf);
		
		printf("id = %d\n",id);
		if(RC_OK != ccAnswerCall(&qcall[id].call_obj,msgbody) ) {
		  //;printf("!!!!!!!!!!!!!ccAnswerCall error\n\n\n");
		} 
		char rsdp_buf[SESS_BUFF_LEN];
		sesslen=SESS_BUFF_LEN-1;
		SessToStr(qcall[id].rsdp,rsdp_buf,&sesslen);
		printf("---rsdp_buf---\n%s\n",rsdp_buf);

		QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
		QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
		QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
		QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
		QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);
		
		QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
		QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
		
		if (0==mode){
			rvideo_port=0;
		}
		
		local_audio_port = qcall[id].qt_call->mCallData.local_audio_port;
		local_video_port = qcall[id].qt_call->mCallData.local_video_port;

        qcall[id].audio_port = local_audio_port;
        qcall[id].video_port = local_video_port;
        
        qcall[id].qt_call->mCallData.is_video_call = qcall[id].media_type;
            
        qcall[id].qt_call->mCallData.start_time = time (NULL);
    
		QStreamPara media;
		set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,local_audio_port,local_video_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
		QuQstreamPrintParameter(&media);
		setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);

	}else{
		
		//invite without SDP,need generate SDP by self
		
		//generate SDP	
		GenAudioMedia(&audio_media);
		
		if (1==mode){
			GenVideoMedia(&video_media);
		}
		
		qcall[id].audio_port=local_audio_port;
		qcall[id].video_port=local_video_port;
		
		SessNew(&sess,profile.ip,audio_media,0);//bandwidth 2048kbps
		SessAddBandwidth(sess,"CT",media_bandwidth);
	
		if(NULL!=video_media){	
			SessAddMedia(sess,video_media);
		}
		if (SessToStr(sess,sessbuf,&sesslen)==RC_OK) //output string
			SessFree(&sess);//printf("sess sdp:\n%s\n",sessbuf);
		
		SessNewFromStr(&qcall[id].lsdp,sessbuf);
		
		printf("Answer 200OK SDP:\n%s",sessbuf);
		ccContNew(&msgbody,"application/sdp",sessbuf);
		ccAnswerCall(&qcall[id].call_obj,msgbody);
		
	}
	
	ccContFree(&msgbody);

	qcall[id].callstate=ACTIVE;
    
    //2012/03/12 added by nicky , record callid
	char callid[128];
	UINT32 callid_len=127;
	ccCallGetCallID(&qcall[id].call_obj,callid,&callid_len);
	qcall[id].callid=strdup(callid);
    
    /*save sip CalldID and iscaller in icall struct for video archive*/
    qcall[id].qt_call->mCallData.is_video_arch_caller = false;
    strcpy (qcall[id].qt_call->mCallData.call_id,callid);
    strcpy (qcall[id].qt_call->mCallData.local_id,profile.sipid);
    
	g_inviteProgress=FALSE;

    qcall[id].is_rev_ack = false;
    int *p_id = (int*) malloc(sizeof(int));
    *p_id = id;
    pthread_t ackExpire;
    pthread_create(&ackExpire, NULL, QuAckExpireThread, (void*) p_id);
    pthread_detach(ackExpire);
    
	return RC_OK;

}

int sip_terminate_call(char* addr)
{
	int id;
	
	if (addr==NULL || *addr==0){
		printf("sip_terminate_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	/*2011-03-01 */
	//g_isconference=FALSE;
	
	id = QuGetCall(addr);
	if (-1==id){
		return RC_ERROR;
	}
	printf("sip_terminate_call addr=%s,id = %d\n",addr,id);
	
	/*2010-08-03 send BYE to original outbound url*/
	//printf("sip_terminate_call:outbound_url=%s\n",qcall[id].sip_sendip);
    
	//ccCallSetOutBound(&qcall[id].call_obj,qcall[id].sip_sendip,profile.ip,LOCALPORT);
	ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port);
    
	if (strcmp("sip:0.0.0.0:5060",qcall[id].sip_sendip)){
		if (RC_OK!=ccDropCall(&qcall[id].call_obj,NULL)){
			printf("ccDropCall NOT OK\n");
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    		pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
    		pthread_mutex_unlock(&qu_free_mutex);
    		QuFreeCall(id,false,"");
			g_inviteProgress=FALSE;

		}else{
			printf("ccDropCall OK\n");
		}
	}
    
    qcall[id].sip_terminate_call=true;
	return RC_OK;
}

int sip_terminate_call_by_id(int callid)
{
	int id;

	if (callid < 0){
		printf("[Error]:sip_terminate_call_by_id:callid -1\n");
		return RC_ERROR;
	}

	id = callid;

	printf("sip_terminate_call_by_id addr=%s,id = %d\n",qcall[id].phone,id);

	/*2010-08-03 send BYE to original outbound url*/
	//printf("sip_terminate_call:outbound_url=%s\n",qcall[id].sip_sendip);

	//ccCallSetOutBound(&qcall[id].call_obj,qcall[id].sip_sendip,profile.ip,LOCALPORT);
	ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port);

	if (strcmp("sip:0.0.0.0:5060",qcall[id].sip_sendip)){
		if (RC_OK!=ccDropCall(&qcall[id].call_obj,NULL)){
			printf("ccDropCall NOT OK\n");
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    		pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
    		pthread_mutex_unlock(&qu_free_mutex);
    		QuFreeCall(id,false,"");
			g_inviteProgress=FALSE;

		}else{
			printf("ccDropCall OK\n");
		}
	}

    qcall[id].sip_terminate_call=true;
	return RC_OK;
}

int sip_cancel_call(char* addr)
{
	int id;
	
	if (addr==NULL || *addr==0){
		printf("sip_cancel_call:parameter NULL\n");
		return RC_ERROR;
	}
	printf("sip_cancel_call:addr=%s\n",addr);
	id = QuGetCall(addr);
	
	/*2011-03-01 */
    //	g_isconference=FALSE;
    
	if (-1==id){
		printf("[Error]:sip_cancel_call:callid -1\n");
		return RC_ERROR;
	}
	
	//printf("sip_cancel_call:outbound_url=%s\n",qcall[id].sip_sendip);
    ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port);
	//ccCallSetOutBound(&qcall[id].call_obj,qcall[id].sip_sendip,profile.ip,LOCALPORT);
    
    ccCallState cs;
    ccCallGetCallState(&qcall[id].call_obj,&cs);
    
    for(int retryTime = 0; retryTime < 5 ;retryTime ++) {
        if(cs >= CALL_STATE_PROCEED) break;
        
        printf("\n\n\n!!!!!cs<CALL_STATE_PROCEED\n\n\n");
        sleep(1);
        ccCallGetCallState(&qcall[id].call_obj,&cs);
    }
	
	if (RC_OK!=ccDropCall(&qcall[id].call_obj,NULL)){
		printf("ccDropCall NOT OK\n");
        //2013-1-17 avoid double lock
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
        pthread_mutex_unlock(&qu_free_mutex);
		QuFreeCall(id,false,"");
		g_inviteProgress=FALSE;
	}else{
		printf("ccDropCall OK\n");
	}

	qcall[id].sip_terminate_call=true;
	printf("qcall[id].sip_cancel_call=TRUE;\n");
	
	printf("CancelExpireThread %s,%d\n",addr,id);
	
	int *p_id = (int*) malloc(sizeof(int));
	*p_id = id;

	pthread_t cancelExpire;
	pthread_create(&cancelExpire, NULL, QuCancelExpireThread, (void*) p_id);
	pthread_detach(cancelExpire);
	
	return RC_OK;
}

int sip_cancel_call_by_id(int callid)
{
	int id;

	if (callid < 0){
		printf("[Error]:sip_cancel_call_by_id:callid -1\n");
		return RC_ERROR;
	}

	id = callid;

	printf("sip_cancel_call:addr=%s,id=%d\n",qcall[id].phone,id);

	/*2011-03-01 */
    //	g_isconference=FALSE;

    ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port);

    ccCallState cs;
    ccCallGetCallState(&qcall[id].call_obj,&cs);

    for(int retryTime = 0; retryTime < 5 ;retryTime ++) {
        if(cs >= CALL_STATE_PROCEED) break;

        printf("\n\n\n!!!!!cs<CALL_STATE_PROCEED\n\n\n");
        sleep(1);
        ccCallGetCallState(&qcall[id].call_obj,&cs);
    }

	if (RC_OK!=ccDropCall(&qcall[id].call_obj,NULL)){
		printf("ccDropCall NOT OK\n");
        //2013-1-17 avoid double lock
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
        pthread_mutex_unlock(&qu_free_mutex);
		QuFreeCall(id,false,"");
		g_inviteProgress=FALSE;
	}else{
		printf("ccDropCall OK\n");
	}

	qcall[id].sip_terminate_call=true;
	printf("qcall[id].sip_cancel_call=TRUE;\n");

	int *p_id = (int*) malloc(sizeof(int));
	*p_id = id;

	pthread_t cancelExpire;
	pthread_create(&cancelExpire, NULL, QuCancelExpireThread, (void*) p_id);
	pthread_detach(cancelExpire);

	return RC_OK;
}

int sip_reject_call(char* addr)
{
	int id;
	
	if (addr==NULL ||*addr==0){
		printf("sip_reject_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	id = QuGetCall(addr);
	if (-1==id){
		return RC_ERROR;
	}
	
	printf("sip_reject_call: addr=%s,id=%d\n",addr,id);
	
	//ccRejectCall(&qcall[id].call_obj,603,"BUSY LINE",NULL);
	ccRejectCall(&qcall[id].call_obj,486,"BUSY LINE",NULL);

	QuFreeCall(id,false,"");
	
	g_inviteProgress=FALSE;

	return RC_OK;

}

int sip_reject_call_by_id(int callid)
{
	int id;

	if (callid < 0){
		printf("[Error]:sip_reject_call_by_id:callid -1\n");
		return RC_ERROR;
	}

	id = callid;
	if (-1==id){
		return RC_ERROR;
	}

	printf("sip_reject_call: addr=%s,id=%d\n",qcall[id].phone,id);

	//ccRejectCall(&qcall[id].call_obj,603,"BUSY LINE",NULL);
	ccRejectCall(&qcall[id].call_obj,486,"BUSY LINE",NULL);

	QuFreeCall(id,false,"");

	g_inviteProgress=FALSE;

	return RC_OK;

}

int sip_hold_call(char* addr){
	
	if (addr==NULL || *addr==0){
		printf("sip_hold_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	int id = QuGetCall(addr);
	
	printf("wait_response=%d\n",qcall[id].wait_response);
	//printf("wait_request=%d\n",qcall[id].wait_request);
	
	/*2011-0720 added by nicky;check if wait user response switch*/
	if ((qcall[id].response_mode==RESPONSE_SWITCH_TO_AUDIO) || (qcall[id].response_mode==RESPONSE_SWITCH_TO_VIDEO)||
		(qcall[id].wait_response != WAITRESPONSE_EMPTY) || (qcall[id].g_RequestProgress)){
		 
		if (qcall[id].g_RequestProgress)
			printf("g_RequestProgress=true,sip_sip_call_ack(0),id=%d\n",id);

		return RC_ERROR;
	}
	
	if(HOLD !=qcall[id].callstate && HOLD_BEHOLD !=qcall[id].callstate){
		qcall[id].request_mode=REQUEST_HOLD;
		QuHoldCall(addr);
		qcall[id].wait_response = WAITRESPONSE_HOLD;
			
	}else{
		printf("!!!!!!sip_hold_call,but state is %d\n",qcall[id].callstate);	
	}
	
	printf("sip_hold_call = %s\n", addr);

	return RC_OK;

}

int sip_resume_call(char* addr){
	
	if (addr==NULL || *addr==0 ){
		printf("sip_resume_call:parameter NULL\n");
		return RC_ERROR;
	}
	int id = QuGetCall(addr);
	
	printf("wait_response=%d\n",qcall[id].wait_response);
	//printf("wait_request=%d\n",qcall[id].wait_request);
	
	if ((qcall[id].wait_response != WAITRESPONSE_EMPTY) || (qcall[id].g_RequestProgress)){
		
		if (qcall[id].g_RequestProgress)
			printf("g_RequestProgress=true,sip_resume_call_ack(0),id=%d\n",id);

		return RC_ERROR;
	}
	
	if(qcall[id].wait_response==WAITRESPONSE_EMPTY) {
		
		if (HOLD != qcall[id].callstate && HOLD_BEHOLD != qcall[id].callstate){
			printf("!!!!!!sip_resume_call,but state is %d\n",qcall[id].callstate);	
		}else{
			QuUnHoldCall(addr);
			qcall[id].wait_response = WAITRESPONSE_RESUME;
		}
		
	}
	
	printf("sip_resume_call = %s\n", addr);

	return RC_OK;
}

int sip_request_switch(int mode, char* addr)
{
	if (mode <0 || mode >1 || addr==NULL || *addr==0){
		printf("sip_switch_call_mode:parameter NULL\n");
		return RC_ERROR;
	}
	
	int id = QuGetCall(addr);
	if (id==-1){
		printf("Error!call id is not exit");
		return RC_ERROR;
	}
	
	if (qcall[id].isquanta == NONE_QUANTA){
		printf("non-quanta device ,reject sip_request_switch\n");
		//return RC_ERROR;
	}
	
	if (qcall[id].callstate == BEHOLD || 
		qcall[id].response_mode==RESPONSE_SWITCH_TO_VIDEO || 
		qcall[id].response_mode==RESPONSE_SWITCH_TO_AUDIO || 
		qcall[id].request_mode==REQUEST_SWITCH_TO_AUDIO || 
		qcall[id].request_mode==REQUEST_SWITCH_TO_VIDEO || 
		g_inviteProgress ){
		
		printf("qcall->callstat=%d,qcall->response_mode=%d,qcall->request_mode=%d,g_inviteProgress=%d\n",
			qcall[id].callstate,qcall[id].response_mode,qcall[id].request_mode,g_inviteProgress);
		return RC_ERROR;
	}
	
	 ccCallState cs;
	 ccCallGetCallState(&qcall[id].call_obj,&cs);

	 if(cs != CALL_STATE_CONNECT){
		 printf("ccCallstat=%d\n",cs);
		 return RC_ERROR;
	 }

	g_inviteProgress=TRUE;
	
	printf("sip_request_switch mode =%d ,addr=%s\n",mode,addr);
	QuSwitchCallMode(mode , addr);

	return RC_OK;

}

//callmode = answer_mode
//mode = request_mode
int sip_confirm_switch(int accept, int mode, char* addr)
{
	if (accept<0 || accept>1 || mode <0 || mode >1 
		|| addr==NULL || *addr==0){
		printf("sip_switch_call_mode:parameter NULL\n");
		return RC_ERROR;
	}
	char remote_a_ip[32]="0";
	char remote_v_ip[32]="0";
	int laudio_port=0,lvideo_port=0;
	int raudio_port=0,rvideo_port=0;
	
	int callmode;
	char sessbuf[SESS_BUFF_LEN]="0";
	UINT32 sesslen=SESS_BUFF_LEN-1;
	pCont msgbody=NULL;
	SipRsp rsp=NULL;
	
	MediaCodecInfo media_info;
	MediaCodecInfoInit(&media_info);
	
	int id = QuGetCall(addr);
	if (id==-1){
		return RC_ERROR;
	}
	
	printf("response mode = EMPTY\n");
	qcall[id].response_mode=RESPONSE_EMPTY;
	
	/*QuoffertoAnswer has already add version*/
	//SessAddVersion(qcall[id].lsdp);//add sdp version	
	
	if (1==accept){
		INT32 media_size;
		SessGetMediaSize(qcall[id].lsdp,&media_size);
		int i;
		pSessMedia sess_media=NULL;
		MediaType media_type;

	    for (i=0;i<media_size;i++){
		    SessGetMedia(qcall[id].lsdp,i,&sess_media);
		    SessMediaGetType(sess_media,&media_type);
		    if (media_type==MEDIA_VIDEO){
				if (RC_OK==SessMediaSetPort(sess_media,qcall[id].video_port))
				    printf("change Video Port\n");
			    else printf("ERROR\n");
		    }
	    }

		SessToStr(qcall[id].lsdp,sessbuf,(INT32*)&sesslen); //output string
		ccContNew(&msgbody,"application/sdp",sessbuf);
	
		QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
		QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
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
					

		sesslen=SESS_BUFF_LEN-1;
		ccContGetBody(&msgbody,sessbuf,&sesslen);
		
		if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,200,"ok",msgbody,qcall[id].reqid,qcall[id].dlg)==RC_OK){
			printf("Accept Switch mode=%d\n",mode);
			qcall[id].media_type=mode;
		}
		//send 200 ok , if accept switch, before qstream */
		//printf("%s\n",sipRspPrint(rsp));
		if (RC_OK==ccSendRsp(&qcall[id].call_obj,rsp)){
			;//printf("ccSendRsp PK\n");
		}
		/*
		printf("QuStreamSwitchCallMode:session=%d, Mode=%d,A_IP=%s,V_IP=%s,laudio port=%d,lvideo port=%d,raudio port=%d,rvideo port=%d,audio_codec=%d %s,video_codec=%d %s\n",
		qcall[id].session,callmode,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,audio_codec,audio_payload,video_codec,video_payload);
		*/
		QStreamPara media;
		set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,callmode,qcall[id].multiple_way_video);
		setQTMediaStreamPara(media,qcall[id].qt_call,qcall[id].media_type);

	}else{
	    //mode = 0 still video
	    //mode = 1 still audio
		int i;
		MediaType media_type;
		int media_size;
		pSessMedia sessmedia=NULL,video_media=NULL;
		SessGetMediaSize(qcall[id].lsdp,&media_size);
		
		for (i=media_size-1;i>=0;i--){
			SessGetMedia(qcall[id].lsdp,i,&sessmedia);
			SessMediaGetType(sessmedia,&media_type);
				
			if (media_type==MEDIA_VIDEO){
				SessRemoveMedia(qcall[id].lsdp,i);
			}
		}
		
		if(mode==0)
		{
		    printf("qcall[id].media_type=%d\n",qcall[id].media_type);
		    qcall[id].media_type = VIDEO;
		    GenVideoMedia(&video_media);
		    SessAddMedia(qcall[id].lsdp,video_media);
		}
		
		SessToStr(qcall[id].lsdp,sessbuf,(INT32*)&sesslen); //output string
		ccContNew(&msgbody,"application/sdp",sessbuf);
		
		QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);
		QuGetMediaCodecInfo(qcall[id].lsdp,&media_info);
		QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
		QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
		QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

		//printf("before rvideo_port ==> rvideo_port = %d\n",rvideo_port);
		if(mode==1) {
		    rvideo_port=0;
		}
		
		if((0!=rvideo_port)&&(-1!=media_info.video_payload_type)){
			callmode=VIDEO;
			qcall[id].multiple_way_video=2;
		}else{
			callmode=AUDIO;
			qcall[id].multiple_way_video=0;
		}
		
		laudio_port = qcall[id].audio_port;
		lvideo_port = qcall[id].video_port;
		
		//if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,486,"Not Accept",msgbody,qcall[id].reqid,qcall[id].dlg)==RC_OK){
		
		if(ccNewRspFromCall(&qcall[id].call_obj,&rsp,406,"Not Accept",NULL,qcall[id].reqid,qcall[id].dlg)==RC_OK){
			printf("Reject Switch(406 Not Accept)\n");
		}
		
		/*send 406 not accept , if not accept switch */
		ccSendRsp(&qcall[id].call_obj,rsp);
	}

	//ccSendRsp(&qcall[id].call_obj,rsp);
	sipRspFree(rsp);
	
	ccContFree(&msgbody);
	
	g_inviteProgress=FALSE;
	qcall[id].g_RequestProgress= FALSE;
	
	return RC_OK;
}

void sip_request_idr(int call_id){
	
	char buffer[256];
	int bufflen=255;
	pCont content=NULL;
	SipReq req=NULL;
	
	printf("sip_request_idr call_id = %d\n",call_id);
		
	if(call_id==-1) {
		return;
	} 
	memset(buffer,0,bufflen);
	snprintf(buffer,bufflen*sizeof(char),"<media_control><vc_primitive><to_encoder><picture_fast_update/></to_encoder></vc_primitive></media_control>\r\n");
	
	ccContNew(&content,"application/media_control+xml",buffer);
	pSipDlg inviteDlg;
	ccGetInviteDlg(&qcall[call_id].call_obj,&inviteDlg);
	
	if (qcall[call_id].call_obj==NULL){
		printf("[Error]sip_request_idr Send SIP_INFO Fail\n");
	}
		
	if (RC_OK==ccNewReqFromCallLeg(&qcall[call_id].call_obj,&req,SIP_INFO,NULL,content,&inviteDlg))
	{
		ccSendReq(&qcall[call_id].call_obj,req,inviteDlg);
		sipReqFree(req);
	}else{
		printf("sip_request_idr Send SIP_INFO Fail\n");
	}
	
	ccContFree(&content);
}

void sip_unattend_transfer(char* addr, char* target_addr)
{
	
	int id;
	char targeturl[256];
	printf("sip_unattend_transfer\n");
	
	if (addr==NULL ||*addr==0 || target_addr==NULL ||*target_addr==0){
		printf("sip_set_call_unattend_transfer:parameter NULL\n");
		return;
	}
	
	id=QuGetCall(addr);
	if (-1==id){
		return;
	}
	memset(targeturl,0,sizeof(targeturl));
	snprintf(targeturl,256*sizeof(char),"sip:%s@%s",target_addr,profile.proxy_server );
	
	ccTransferCall(&qcall[id].call_obj,targeturl,NULL,NULL);
	
    qcall[id].transfer_mode = UNATTEND_TRANSFER;
    
	g_inviteProgress=FALSE;
	
	
}

int sip_transfer_terminate_call(char* addr)
{
	int id;
	
	if (addr==NULL || *addr==0){
		printf("sip_terminate_call:parameter NULL\n");
		return RC_ERROR;
	}
	
	/*2011-03-01 */
	//g_isconference=FALSE;
	
	id = QuGetCall(addr);
	if (-1==id){
		return RC_ERROR;
	}
	
    /*2010-08-03 send BYE to original outbound url*/
    
    printf("sip_transfer_terminate_call:outbound_url=%s\n",qcall[id].sip_sendip);
    //ccCallSetOutBound(&qcall[id].call_obj,qcall[id].sip_sendip,profile.ip,LOCALPORT);
    ccCallSetOutBound(&qcall[id].call_obj,outbound_url,profile.ip,g_local_listen_port);
    
    if (strcmp("sip:0.0.0.0:5060",qcall[id].sip_sendip)){
        
        if (RC_OK!=ccDropCall(&qcall[id].call_obj,NULL)){
            printf("ccDropCall NOT OK\n");
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    		pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
    		pthread_mutex_unlock(&qu_free_mutex);
    		QuFreeCall(id,false,"");
            g_inviteProgress=FALSE;
        }else{
            printf("ccDropCall OK\n");
        }
        
    }
    
    g_inviteProgress=FALSE;
        
    return RC_OK;
}

void EvtCB(CallEvtType event, void* msg)
{
	pCCMsg	postmsg=(pCCMsg)msg;
	SipReq	req=NULL;
	SipRsp	rsp=NULL;
	pSipDlg callleg = NULL;
	pCCCall call;
	SipMethodType	method;

	ccCallState cstate;
	int statuscode;

	if (!b_itri_init) {
		return;
	}

	ccMsgGetCALL(&postmsg,&call);
	ccMsgGetReq(&postmsg,&req);
	ccMsgGetCallLeg(&postmsg,&callleg);
	
	char *displayname;
	char phonenumber[64];
	int id=-1;
	int res;
	
	memset(phonenumber,0,sizeof(phonenumber));
	//printf("event=%d\n",event);
	switch(event) {
		case CALL_STATE:

			ccCallGetCallState(&call,&cstate);

			if(rsp && (GetRspMethod(rsp)==INVITE)){
				ccMsgGetRsp(&postmsg,&rsp);
				displayname = GetRspToDisplayname(rsp);
				if (!displayname){
					displayname=strdup("");
				}
				getPhoneToRsp(rsp,phonenumber);
				id=QuGetCall(phonenumber);
				if (-1==id){
					return;
				}
				if (qcall[id].flow_status==PRE_TRANSFER){
				
					SipReq notify=NULL;
					pCont sipfragmsg=NULL;
					ccContNew(&sipfragmsg,"message/sipfrag;version=2.0","SIP/2.0 200 OK\r\n");
					if(RC_OK==ccNewReqFromCallLeg(&qcall[id].refer_obj,&notify,SIP_NOTIFY,NULL,sipfragmsg,&qcall[id].refer_dlg)){
						ccContFree(&sipfragmsg);
						//add event and subscription-state
						ccReqAddHeader(notify,Event,"refer");

						int sc=GetRspStatusCode(rsp);
						if(sc>=200)
							printf("terminated;reason=noresource\n");
						
						ccReqAddHeader(notify,Subscription_State,"active;expires=180");
						ccSendReq(&qcall[id].refer_obj,notify,qcall[id].refer_dlg);
						sipReqFree(notify);
					}
					ccContFree(&sipfragmsg);
				}
			}

			if(cstate==CALL_STATE_INITIAL){
				printf("CALL_STATE_INITIAL\n");
			}else if(cstate==CALL_STATE_PROCEED){
				printf("CALL_STATE_PROCEED\n");	
				//printf("id=%d,%d\n",id,qcall[id].qt_call);
				//g_event_listner->onCallOutgoing(qcall[id].qt_call);
			}else if(cstate==ITRI_CALL_STATE_RINGING){
				printf("ITRI_CALL_STATE_RINGING\n");
				pSipDlg dlg;
				SipReq prackreq=NULL;
				pCont content=NULL;
				ccMsgGetRsp(&postmsg,&rsp);
				getPhoneToRsp(rsp,phonenumber);

				
				if(Be100rel(rsp)){
					ccMsgGetCallLeg(&postmsg,&dlg);
					if(ccNewReqFromCallLeg(&call,&prackreq,PRACK,NULL,content,&dlg)==RC_OK){
						//printf("\n%s\n",sipReqPrint(prackreq));
						char usercontact[256];
						memset(usercontact,0,256*sizeof(char));
						snprintf(usercontact,256*sizeof(char),"<sip:%s@%s:5060>",profile.sipid,profile.ip);
						ccReqAddHeader(prackreq,Contact,usercontact);
						
						if(sipReqGetHdr(prackreq,RAck)!=NULL){
							ccSendReq(&call,prackreq,dlg);
						}
						sipReqFree(prackreq);
					}
				}

				id=QuGetCall(phonenumber);
				if(g_event_listner != NULL && qcall[id].qt_call != NULL)
				{
					// Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
					pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                
					qcall[id].qt_call->mCallData.call_state = call::CALL_STATE_RINGING;
					//strcpy(qcall[id].qt_call->mCallData.call_id, phonenumber); //mark by nicky
					printf("g_event_listner->onCallStateChanged  %d\n", qcall[id].qt_call);
					g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, ""); 
					
					//Leave the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
				 pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
				}
				
			}else if(cstate==CALL_STATE_ANSWERED){
				printf("CALL_STATE_ANSWERED\n");
				ccMsgGetRsp(&postmsg,&rsp);
				getPhoneToRsp(rsp,phonenumber);
				
				pSess sess=NULL;
				pCont content=NULL;
				char remote_a_ip[32]="0";
				char remote_v_ip[32]="0";
				int laudio_port=0,lvideo_port=0;
				int raudio_port=0,rvideo_port=0;
			
				MediaCodecInfo media_info;
				MediaCodecInfoInit(&media_info);
			
				int callmode=0;
				displayname = GetRspToDisplayname(rsp);
				if (!displayname){
					displayname=strdup("");
				}

				id=QuGetCall(phonenumber);
				printf("callee answer:id=%d,phone=%s\n",id,phonenumber);
				if (-1==id){
					return;
				}
                
                g_isMCUConference = false;
                
				ccContNewFromRsp(&content,rsp);
			
				UINT32 contlen=0;
				ccContGetLen(&content,&contlen);
				printf("contlen=%d\n",contlen);

				char *contbuf;
				bool is_reinvite_codec=false;
				
				if (0 < contlen){
					contbuf = (char*) malloc (contlen+8);
					memset(contbuf,0, contlen+8);

					QuContToSessToStr(content,contbuf,&contlen);
					ccContFree(&content);
					content=NULL;

					if (NULL!=qcall[id].rsdp){
						SessFree(&(qcall[id].rsdp));
					}
					
					SessNewFromStr(&qcall[id].rsdp,contbuf);
					free(contbuf);
					
					qcall[id].callstate=ACTIVE;//session expires handle
					
					QuGetRemoteMediaInfo(qcall[id].rsdp,remote_a_ip,remote_v_ip,&raudio_port,&rvideo_port);

					if (check_answer_completely(qcall[id].rsdp)){
						int sesslen=SESS_BUFF_LEN-1;
						char sessbuf[SESS_BUFF_LEN]="0";
						//unsigned long oldversion,newversion;
						//SessToStr(qcall[id].lsdp,sessbuf,&sesslen);
						//printf("old @@\n%s\n",sessbuf);
						//SessGetVersion(qcall[id].lsdp,&oldversion);
						//QuReInviteOfferToAnswer(id,qcall[id].rsdp);
						QuNewOfferToAnswerToStr(qcall[id].rsdp,id,sessbuf,&sesslen);
						//SessGetVersion(qcall[id].lsdp,&newversion);
						//printf("old version =%lu , new version =%lu\n",oldversion,newversion);
						//sesslen=1023;
						//SessToStr(qcall[id].lsdp,sessbuf,&sesslen); //output string
						printf("new @@\n%s\n",sessbuf);
						//ccContNew(&content,"application/sdp",sessbuf);
						pSess sess=NULL;
						SessNewFromStr(&sess,sessbuf);
						
						QuGetMediaCodecInfo(sess,&media_info);
						
						is_reinvite_codec=true;
						SessFree(&sess);
					}else{
						QuGetMediaCodecInfo(qcall[id].rsdp,&media_info);
					}

					QuGetRemoteH264CodecInfo(qcall[id].rsdp,&media_info);
					QuGetLocalMediaSRTPInfo(qcall[id].lsdp,&media_info);
					QuGetRemoteMediaSRTPInfo(qcall[id].rsdp,&media_info);

					if((0!=rvideo_port)&&(-1!=media_info.video_payload_type)){
						callmode=VIDEO;
						qcall[id].media_type=VIDEO;
					}else{
						callmode=AUDIO;
						qcall[id].media_type=AUDIO;
					}
					
					laudio_port = qcall[id].audio_port;
					lvideo_port = qcall[id].video_port;
					
					QuAudioCodecStrtoInt(media_info.audio_payload,&qcall[id].audio_codec);
					QuVideoCodecStrtoInt(media_info.video_payload,&qcall[id].video_codec);
					
					if (sipRspGetHdr(rsp,P_Asserted_Identity)!=NULL){
						SipIdentity *p=(SipIdentity*)sipRspGetHdr(rsp,P_Asserted_Identity);

						char p_phone[64];
						char p_display_name[64];
						if (p->address->display_name!=NULL){
							sscanf(p->address->display_name,"\"%[^\"]",p_display_name);
						}else{
							strcpy(p_display_name,"");
						}
						
						if (p->address->addr!=NULL){
							sscanf(p->address->addr,"%*[^:]:%[^@]",p_phone);
						}else{
							strcpy(p_phone,"");
						}
						
						printf("P-Asserted-Identity:phone=%s,display_name=%s,addr=%s\n",phonenumber,p_display_name,p_phone);
					
					}

					/* identy is Quanta product*/
					QuSetIsQuantaProduct(id, qcall[id].rsdp);
					QTSetIsQuantaProduct(id);
					

					SipContact *pContact;
					pContact=(SipContact*)sipRspGetHdr(rsp,Contact);
					bool is_ims_conference = false;

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
								printf("name = %s ",parameter->name);
								if(!strncmp(parameter->name,"isfocus",7))
								{
									printf("is_ims_conference=true\n");
									qcall[id].isquanta = QUANTA_PTT;
									QTSetIsQuantaProduct(id);
								}
							}
							if(parameter->value)
							{
								printf("value = %s",parameter->value);
							}
							printf("\n");
					
							if(parameter->next) parameter = parameter->next;
						}

					}

					QStreamPara media;

					set_media_para(&media,qcall[id].session,qcall[id].isquanta,remote_a_ip,remote_v_ip,laudio_port,lvideo_port,raudio_port,rvideo_port,&media_info,qcall[id].media_type,qcall[id].multiple_way_video);
					QuQstreamPrintParameter(&media);

					setQTMediaStreamPara(media , qcall[id].qt_call , qcall[id].media_type);
				}
                
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
					memset(audio_mime, 0, 16*sizeof(char));
					memset(video_mime, 0, 16*sizeof(char));
					strcpy(audio_mime, qcall[id].qt_call->mCallData.audio_mdp.mime_type);
					strcpy(video_mime, qcall[id].qt_call->mCallData.video_mdp.mime_type);

					//qcall[id].qt_call->mCallData.is_caller=false; //mark by nicky 2013/10/03
					qcall[id].qt_call->mCallData.is_video_call = qcall[id].media_type;
					qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
					
                    if (g_isMCUConference){
                        printf("[SIP]!!!!MCU mode\n");
                        if (QuCheckInSession()){
                            // end qstream and send bye
                            qcall[1-id].qt_call->mCallData.is_session_start = false;
                            printf("g_event_listner->onCallAnswered %d\n",qcall[id].qt_call);
                            g_event_listner->onCallAnswered(qcall[id].qt_call);
                            printf("g_event_listner->onMCUSessionEnd %d\n",qcall[1-id].qt_call);
                            g_event_listner->onMCUSessionEnd(qcall[1-id].qt_call);
                            
                        }else{
                            printf("g_event_listner->onCallAnswered %d\n",qcall[id].qt_call);
                            g_event_listner->onCallAnswered(qcall[id].qt_call);
                        }
                        
                    }else{
                        printf("g_event_listner->onCallAnswered %d\n",qcall[id].qt_call);
                        g_event_listner->onCallAnswered(qcall[id].qt_call);
                    }
                    qcall[id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
					

                    qcall[id].qt_call->mCallData.is_session_start = true;
                    qcall[id].qt_call->mCallData.start_time = time (NULL);
                    
                    printf("**** is_caller=%d\n",qcall[id].qt_call->mCallData.is_video_arch_caller);
                    
                    if (g_isMCUConference){
                    	printf("g_event_listner->onMCUSessionStart %d\n",qcall[id].qt_call);
                        g_event_listner->onMCUSessionStart(qcall[id].qt_call);
                    }else{
                    	printf("g_event_listner->onSessionStart %d\n",qcall[id].qt_call);
                        g_event_listner->onSessionStart(qcall[id].qt_call);
                    }

					// Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
					pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);

				}

				g_inviteProgress=FALSE;
				ccAckCall(&call,NULL);

			}else if(cstate==CALL_STATE_CONNECT){
				printf("CALL_STATE_CONNECT\n");
				
				char phonenumber[64]="0";
				getPhoneToReq(req,phonenumber);
				int call_id = QuGetCall(phonenumber);
				printf("cstate==CALL_STATE_CONNECT phonenumber=%s,id=%d\n",phonenumber,call_id);

				QuRevAck(req);

				if (call_id==-1){
					break;
				}
                if (qcall[call_id].sip_terminate_call){
                    if(RC_OK!=ccDropCall(&qcall[call_id].call_obj, NULL)){
                        QuFreeCall(call_id,false,"");
                        g_inviteProgress=false;
                    }
                }
				//qcall[call_id].qt_call->mCallData.call_state = CALL_STATE_ACCEPT;
			}else if (cstate==CALL_STATE_OFFER) {

				printf("CALL_STATE_OFFER\n");

				pthread_mutex_lock ( &qu_callengine_mutex );

				int call_id = 0 ;
				call_id = QuPreAnswerCall(call ,req ,callleg);
				printf("call_id=%d\n",call_id);
				if (call_id != -1) {
					//Create new call

					itricall::ItriCall *pcall = new itricall::ItriCall();
					//ItriCall *pcall = new ItriCall();
	                
					//copy remote id to call
					memset(pcall->mCallData.remote_id,0,128*sizeof(char));
					//strcpy(pcall->mCallData.remote_id,"unknown");
					/*sip_p_asserted_identity_t *passerted = sip_p_asserted_identity(sip);
					if (passerted && passerted->paid_url && passerted->paid_url->url_user)
						strcpy(pcall->mCallData.remote_id,passerted->paid_url->url_user);*/
	                
					//parse remote id from P-Asserted-Identity
	               
					strcpy(pcall->mCallData.remote_id,qcall[call_id].phone);
	                
					//copy display name to icall
					memset(pcall->mCallData.remote_display_name,0,128*sizeof(char));
					strcpy(pcall->mCallData.remote_display_name,qcall[call_id].display_name);
	                
					//copy remote domain to call
					//memset(pcall->mCallData.remote_domain,0,sizeof(pcall->mCallData.remote_domain));
					//strcpy(pcall->mCallData.remote_domain,sip->sip_from->a_url[0].url_host);
	                
					//copy call id to icall
					//memset(pcall->mCallData.call_id,0,sizeof(pcall->mCallData.call_id));
					//strcpy(pcall->mCallData.call_id, sip->sip_call_id->i_id);
	                
					//pcall->mCallData.call_handle = nh;
					pcall->mCallData.call_type   = CALL_TYPE_INCOMING;
					pcall->mCallData.is_video_call = qcall[call_id].media_type;
					pcall->mCallData.call_state  = call::CALL_STATE_RINGING;
					//add to call list
					qcall[call_id].qt_call = pcall;
					//if(g_event_listner != NULL) {
                    if(g_event_listner != NULL && qcall[call_id].qt_call != NULL)
                    {
                        // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[call_id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[call_id].qt_call->mCallData.calldata_lock);
                        printf("g_event_listner=%x\n",g_event_listner);
                        switch(qcall[call_id].isquanta){
                        	case QUANTA_MCU:
                        		//if device is in-session now and caller's referby is the same with another phone
								//MCU Auto answer
								printf("[SIP]MCU mode: check\n");
								if (QuCheckInSession()){
									printf("[SIP]MCU mode: QuCheckInSession True\n");
									//delete old Icall table

									printf("onMCUSessionEnd %d\n",qcall[1-call_id].qt_call);
									g_event_listner->onMCUSessionEnd(qcall[1-call_id].qt_call);

									if (qcall[1-call_id].qt_call !=NULL){
										delete qcall[1-call_id].qt_call;
									}
									qcall[1-call_id].qt_call =NULL;
								}
								printf("onCallMCUIncoming %d\n",qcall[call_id].qt_call);
								g_event_listner->onCallMCUIncoming(qcall[call_id].qt_call);

                        		break;
                        	case QUANTA_PTT:
                        		printf("onCallPTTIncoming %d\n",qcall[call_id].qt_call);
                        		g_event_listner->onCallPTTIncoming(qcall[call_id].qt_call);

                        		break;
                        	default:
                        		printf("onCallIncoming %d\n",qcall[call_id].qt_call);
                        		g_event_listner->onCallIncoming(qcall[call_id].qt_call);
                        		break;
                        }
                        printf("onCallStateChanged %d\n",qcall[call_id].qt_call);
						g_event_listner->onCallStateChanged(qcall[call_id].qt_call, qcall[call_id].qt_call->mCallData.call_state, "");
                        
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[call_id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[call_id].qt_call->mCallData.calldata_lock);
					}
				}
                
				pthread_mutex_unlock ( &qu_callengine_mutex );


			}else if(cstate==CALL_STATE_ANSWER){
				printf("CALL_STATE_ANSWER\n");
			}else if(cstate==CALL_STATE_DISCONNECT){
				printf("CALL_STATE_DISCONNECT\n");

				ccMsgGetRsp(&postmsg,&rsp);
				statuscode=GetRspStatusCode(rsp);
				printf("statuscode = %d\n",statuscode);
				method=GetReqMethod(req);
				
				statuscode==-1 ? getPhoneFromReq(req,phonenumber)
						   : getPhoneToRsp(rsp,phonenumber);
					       
				int id = QuGetCall(phonenumber);
				if(id == -1) {
					statuscode==-1 ? getPhoneToReq(req,phonenumber)
						   : getPhoneFromRsp(rsp,phonenumber);
				}
									
				if (BYE == method){
					
					if (statuscode == -1){
						printf("Receive BYE!\n");
						QuRevBye(req);
					}else {
						printf("Receive BYE 200OK!\n");
						QuRevBye200OK(rsp);
					}
					
				}else if((method==INVITE)&&(statuscode>=300)){
					if (statuscode == 302) {
						char *newtarget=NULL;
						newtarget=GetRspContact(rsp);
						int originalID;
						char *originalPhone=sipURLGetUser(sipURLNewFromTxt(GetRspToUrl(rsp)));
						char *newtargetPhone=sipURLGetUser(sipURLNewFromTxt(newtarget));
						originalID=QuGetCall(originalPhone);
						unsigned int mode = (unsigned int) qcall[originalID].media_type;
						
						if(newtarget){
							int i;
						    
							//20111222 junior say it is no risk.
							bool enable_forward=true;
							//2011-11-07
							for(i=0;i<NUMOFCALL;i++){
								if (qcall[i].call_obj!=NULL && !strcmp(qcall[i].phone,newtargetPhone)){
									printf("!!!!!!!<sip> Call Foward other call is with the same callee\n");
									QuInviteFail(rsp);
									enable_forward=false;
								}
							}
							//tmpStr.Format("[Call #%d] redirect to %s",callHandle,newtarget);
							//DebugMsg(tmpStr);
							if (enable_forward){
								printf("<sip> Call Foward: new target %s\n",newtarget);
								//QuInviteIsForwarded(mode,newtargetPhone,rsp);
							}
							//make new call;
							//int newcallhandle;
							//sip_make_call(mode,newtargetPhone,&result);
							//@@MakeCallEx(newtarget,newcallhandle,req);
						}
						
					}else if (401 == statuscode || 407 == statuscode){
						SipReq newreq=NULL;
						char sessbuf[SESS_BUFF_LEN]="0";
						pCont	msgbody=NULL;
					
						newreq=sipReqDup(req);
					
						/*
						ccCallGetUserInfo(&call,&user);
						ccCallGetCallID(&call,callid ,&id_len);
						ccCallNew(&newcall);
						ccCallSetUserInfo(&newcall,user);
						ccCallNewEx(&newcall,callid);
						*/
						ccReqAddAuthz(newreq,rsp,profile.username,profile.password);
						sipReqAddHdr(newreq,CSeq,sipReqGetHdr(req,CSeq));
						
						//copy from
						sipReqAddHdr(newreq,From,sipReqGetHdr(req,From));
						
						if (ccContNewFromReq(&msgbody,req)==RC_OK);
							//printf("ccContNewFromReq SDP OK!!!!\n");
						
						char *requrl = GetReqUrl(req);
						
						ccMakeCall(&call,requrl,msgbody,newreq);
						//ccMakeCall(&newcall,DialURL,msgbody,preq); //preq ccReqAddAuthz()
						sipReqFree(newreq);
						ccContFree(&msgbody);
	
					}else if(statuscode==486){
						//get a busy response
						printf("get a busy response\n");
						QuInviteFail(rsp);
					}else if(statuscode==487){
						printf("receive cancel 487\n");
						
                        char *displayname = GetRspToDisplayname(rsp);
                        if (!displayname){
                            displayname=strdup("");
                        }
                        char phonenumber[64]="0";
                        getPhoneToRsp(rsp,phonenumber);
                        
                        int id = QuGetCall(phonenumber);
                        //int id = QuGetCallFromCallid((char *)sipRspGetHdr(rsp,Call_ID));
                        printf("QuInviteFail id=%d\n",id);
                        
                        if (-1==id){
                            return ;
                        }
                        
                        QuFreeCall(id,false,"");
                        
                        g_inviteProgress=FALSE;
                        
					}else {
						QuInviteFail(rsp);
					}
				}			

			}
			break;
		case SIP_REQ_REV:
			ccMsgGetReq(&postmsg,&req);
#if PRINT_PACKET
            printf("%s\n",sipReqPrint(req));
#endif
			method=GetReqMethod(req);
            
			if (method!=OPTIONS){
				char *displayname;	
				displayname = GetReqFromDisplayname(req);
				if (!displayname){
					displayname=strdup("");
				}
				getPhoneFromReq(req,phonenumber);
			}
			
			if((REFER==method)||(method==SIP_SUBSCRIBE)){
                if(ccNewRspFromCall(&call,&rsp,405,"Method Not Allowed",NULL,GetReqViaBranch(req),callleg)==RC_OK){
                    //ccRspAddHeader(rsp,Privacy,"id"); //privacy extend //rfc3323
                    ccSendRsp(&call,rsp);
                    sipRspFree(rsp);
                }
			}else if (ACK==method) {
                printf("Receive re-invite ACK\n");
                QuRevReAck(req);
			}else if (method==REGISTER){
			
				if(ccNewRspFromCall(&call,&rsp,200,"ok",NULL,GetReqViaBranch(req),NULL)==RC_OK){
				//printf("\n%s\n",sipRspPrint(rsp));
				ccSendRsp(&call,rsp);
				sipRspFree(rsp);
				}
			}else if(PRACK==method){
				pSipDlg dlg;
				//printf("case SIP_REQ_REV: PRACK\n");
				ccMsgGetCallLeg(&postmsg,&dlg);
				if(ccNewRspFromCall(&call,&rsp,200,"ok",NULL,GetReqViaBranch(req),dlg)==RC_OK){
					//printf("\n%s\n",sipRspPrint(rsp));
					ccSendRsp(&call,rsp);
					sipRspFree(rsp);
				}
			}else if(SIP_UPDATE == method){
                pSipDlg dlg;
                ccMsgGetCallLeg(&postmsg,&dlg);
                printf("Receive SIP_UPDATE\n");
                //g_inviteProgress=TRUE;
                QuRevUpdate(call,req,dlg);
                
			}else if(INVITE == method){
				pSipDlg dlg;
				ccMsgGetCallLeg(&postmsg,&dlg);
				printf("Receive re-invite\n");
				//g_inviteProgress=TRUE;
				QuRevReInvite(call,req,dlg);
			}else if(OPTIONS == method){
                pSipDlg dlg;
                //printf("case SIP_REQ_REV: OPTIONS\n");
                ccMsgGetCallLeg(&postmsg,&dlg);
                if(ccNewRspFromCall(&call,&rsp,200,"ok",NULL,GetReqViaBranch(req),dlg)==RC_OK){
                    //printf("\n%s\n",sipRspPrint(rsp));
                    ccRspAddHeader(rsp,Allow,"INVITE,ACK,CANCEL,OPTIONS,BYE" );
                    ccRspAddHeader(rsp,Accept,"application/sdp");
                    ccRspAddHeader(rsp,Accept_Encoding,"identity");
                    ccRspAddHeader(rsp,Accept_Language,"en");
                    ccSendRsp(&call,rsp);
                    sipRspFree(rsp);
                }
			}else if(SIP_INFO == method ){
                
                pSipDlg dlg;
                pCont content=NULL;
                ccMsgGetCallLeg(&postmsg,&dlg);
                ccContNewFromReq(&content,req);
                
                getPhoneFromReq(req,phonenumber);
                
                char conttype[256]="\0";
                UINT32	len=255;
                
                ccContGetType(&content,conttype,&len);
                    
                bool isIFrameRequest = false;
                
                char sessbuf[SESS_BUFF_LEN];
                UINT32 sesslen=SESS_BUFF_LEN-1;
                
                int id=QuGetCall(phonenumber);
                
                if(!strcmp("application/media_control+xml",conttype)){
                    //message body is sdp
                    
                    if(RC_OK==ccContGetBody(&content,sessbuf,&sesslen)){
                        printf("receive an xml info\n");//SessNewFromStr(&sdp,bodybuf);
                        
                        char xmlLabelList[4][20] = {"media_control","vc_primitive","to_encoder","picture_fast_update"};
                        char *pch;
                        
                        int i;
                        pch=strtok(sessbuf," <>\n\t/");
                        for(i=0;i<4;i++) {
                            //printf("%s\n",pch);
                            if(strcmp(xmlLabelList[i] , pch)) i=i-1;
                            if(pch==NULL || *pch=='/') break;
                            pch=strtok(NULL," <>\n\t/");
                        }
                        if(i==4) {
                            
                            isIFrameRequest = true;
                            printf("sip_info phonenumber=%s,id=%d\n",phonenumber,id);
                            printf("onRequestIDR\n");	
                            
                            //for QT
                            if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                            {
                                // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                                pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                                printf("g_event_listner->onRequestIDR %d\n",qcall[id].qt_call);
                                g_event_listner->onRequestIDR(qcall[id].qt_call);
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                                pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                            }

                        }
                    }else{
                        printf("ccContGetBody fail! without body\n");
                        sesslen =0;
                    }
                }else{
                    sesslen =0;
                }
             

                if(ccNewRspFromCall(&call,&rsp,200,"ok",NULL,GetReqViaBranch(req),dlg)==RC_OK){
                    //printf("\n%s\n",sipRspPrint(rsp));
                    ccRspAddHeader(rsp,Allow,"INVITE,ACK,CANCEL,OPTIONS,BYE" );
                    ccRspAddHeader(rsp,Accept,"application/sdp");
                    ccRspAddHeader(rsp,Accept_Encoding,"identity");
                    ccRspAddHeader(rsp,Accept_Language,"en");
                    ccSendRsp(&call,rsp);
                    sipRspFree(rsp);
                }
			}else if(BYE == method){
				printf("BYE!!!!!!!!!!!!!!!!!!!!!\n");
			}else if (SIP_NOTIFY==method){
                printf("Get NOTIFY request\n");
    
                SipRsp rsp;
                
                getPhoneFromReq(req,phonenumber);
                int id=QuGetCall(phonenumber);
                
                if (!strncmp(GetReqEvent(req),"refer",5)){
                    
                    if (RC_OK==ccNewRspFromCall(&call,&rsp,200,"ok",NULL,GetReqViaBranch(req),callleg)){
                        ccRspAddHeader(rsp,Event,"refer");
                        ccRspAddHeader(rsp,Accept_Language,"en");
                        printf("send 200ok for NOTIFY\n");
                        ccSendRsp(&call,rsp);
                    }
                    sipRspFree(rsp);
                    
                    
                    char *token = NULL ;
                    int result=0;
                    char *refer_id_str = (char *)strstr(GetReqEvent(req),"id=");
                    
                    if (refer_id_str){
                        token = strtok( refer_id_str , " =" );
                        token = strtok( NULL , " =" );
                        printf("[SIP]Refer id=%s\n",token);
                    }
                    
                    pCont	content=NULL;
                    ccContNewFromReq(&content,req);
                    
                    int sip_frag_status = GetSipFrag(content);
                    printf("sip_frag_status=%d\n",sip_frag_status);
                    
                    qcall[id].thrd_transfer_notify_expire = TRUE;
                    
                    if (qcall[id].transfer_mode == ATTEND_TRANSFER){
						
                    }else if (qcall[id].transfer_mode == UNATTEND_TRANSFER){
                        if (sip_frag_status>=180 && sip_frag_status<200){
                            result = 0;
                        }else if (sip_frag_status==200){
                            result = 1;
                            /*
                            if (qcall[1-id].callid!=NULL && (!strcmp(token,qcall[1-id].phone))){
                            	printf("[SIP]qcall[1-id].phone=%s\n",qcall[1-id].phone);
                            	sip_terminate_call(qcall[1-id].phone);
                            	//start qstream with mcu server
                            	QuStartQstreamWithCallid(id);
                            }
                             */
                        }else if (sip_frag_status>300){
                            result = 0;
                        }
                        
                        if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                        {
                            // Enter the critical section
#if MUTEX_DEBUG
                            printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                            pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                            if (token){
                                printf("onCallAddConference , id=%s ,success=%d\n",token,result);
                                //g_event_listner->onCallAddConference(qcall[id].qt_call ,token ,result);
                            }
#if MUTEX_DEBUG
                            printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                            pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                        }
                    }
                    
                }
                
            }

			break;
		case SIP_RSP_REV:
			ccMsgGetRsp(&postmsg,&rsp);
#if PRINT_PACKET
            printf("[SIP]SIP_RSP_REV=======>\n");
            printf("%s\n",sipRspPrint(rsp));
#endif
            method=GetRspMethod(rsp);
			//if re-invite,check sdp
            statuscode=GetRspStatusCode(rsp);
            printf("statuscode = %d\n",statuscode);

			SipCSeq *pCseq;
			pCseq = (SipCSeq*)sipRspGetHdr(rsp,CSeq);

			if (pCseq->Method == REGISTER && statuscode > 100){

				printf("old Cseq=%u\n",reg.handle_cseq);
				printf("new Cseq=%u\n",pCseq->seq);

				if (pCseq->seq <= reg.handle_cseq ){
					printf("receive old CSeq , ignore this response\n");
					break;
				}
				reg.handle_cseq = pCseq->seq ;

			}

			//183 Session Progressing
			if(statuscode==183){
				;
			}
			if((statuscode>=200)&&(statuscode<300)){
                
                static bool is_set_onLogonState = false;
				if (REGISTER==method){
					int expires = 0;
					int contact_expires = 0;
					int rsp_expires = 0;
					SipContact *pContact;
					pContact=(SipContact*)sipRspGetHdr(rsp,Contact);
					
                    int register_count=0;;
                    
					while (pContact){
						if(pContact){
							if(pContact->sipContactList){
                                register_count++;
                            	contact_expires = pContact->sipContactList->expireSecond;
                                printf("!!!contact ip =%s\n",sipURLGetDomain(pContact->sipContactList->address->addr));
                                if (pContact->sipContactList->address->addr !=NULL && !strcmp(sipURLGetDomain(pContact->sipContactList->address->addr),profile.ip)){
                                    //if(contact_expires>=pContact->sipContactList->expireSecond){
                                    //printf("test=%s\n",pContact->sipContactList->address->addr);
                                    //}
                                }else{
                                    //double log in, auto log out
                                    /*
                                    pthread_t thrdlogout;
                                    pthread_create(&thrdlogout, NULL, logout, NULL);
                                    printf("[WARNING]=double login,addr=%s\n",pContact->sipContactList->address->addr);
                                    
                                    return;
                                    */
                                }
                            }
						}	
						
						if (pContact->sipContactList->next)
							pContact->sipContactList= pContact->sipContactList->next;
						else
							break;
					}
					

					rsp_expires = GetRspExpires(rsp);

                    
                    if (-1 == contact_expires && -1 == rsp_expires) {
                        expires = 0;
                    }else{
                    	if (contact_expires > rsp_expires){
                    		expires = contact_expires;
                    	}else{
                    		expires = rsp_expires;
                    	}
                    }
                    printf("register expires= %d\n",expires);
                    reg.re_transmit=0;
                    /*
                    if (register_count>=2){
                        pthread_t thrdlogout;
                        pthread_create(&thrdlogout, NULL, logout, NULL);
                        printf("[WARNING]=double login,addr=%s\n",pContact->sipContactList->address->addr);
                        g_event_listner->onLogonState(false,"Multiple Login", expires, profile.ip);
                        return;
                    }
                    */
                    if (expires==0){
                        //g_event_listner->onLogonState(false,"Login fail", expires, profile.ip);
                        return;
                    }
					if (!reg.startkplive) {
                        
                        /*if sip is in session , do not notify UI level*/
                        //if ((qcall[0].call_obj==NULL) && (qcall[1].call_obj==NULL)) {
                            printf("SIP is not in session\n");
                            printf("is_set_onLogonState=%d\n",is_set_onLogonState);
                            
                            //if (!is_set_onLogonState)
                                //g_event_listner->onLogonState(true,"Login Successfully", expires, profile.ip);
                            
                            //is_set_onLogonState = true;
                            if(g_event_listner != NULL){
                            	printf("g_event_listner->onLogonState login successfully\n");
                            	printf("g_event_listner=%x\n",g_event_listner);
                            	g_event_listner->onLogonState(true,"Login Successfully", expires, profile.ip);
                            }
                        //}
                        
                        /*alway notify UI level 2012/07/31*/
                        //g_event_listner->onLogonState(true,"Login Successfully", expires, profile.ip);
                        
						se.regreq=sipReqDup(req);

						se.expires = expires;
						memcpy(&se.regcall , &reg.regcall , sizeof(reg.regcall));
						memcpy(&se.user , &reg.user , sizeof(reg.user));

						char buf[32]="0";
						UINT32 buflen=31;
						ccUserGetUserName(&reg.user,buf,&buflen);
                        printf("se.expires=%d\n",se.expires);
						pthread_create(&reg.threadRegister, NULL, kpliveThread, &se);
						reg.startkplive = TRUE;
						reg.is_register = statuscode;

						printf("******************************\n");
						printf("*\n");
						printf("*Register Success:%s\n",buf);
						printf("*\n");				
						printf("******************************\n");

					}
				}else if (INVITE==method){
					getPhoneToRsp(rsp,phonenumber);
					id=QuGetCall(phonenumber);
					printf("Receive reinvite 200OK  id=%d , call_state=%d\n",id,qcall[id].qt_call->mCallData.call_state);
					QuRevReInvite200OK(call,rsp);
                    
                    if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                    {
                        // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                        
                        qcall[id].qt_call->mCallData.is_reinvite = false;
                        qcall[id].qt_call->mCallData.is_caller = false;
                        printf("g_event_listner->onCallStateChanged %d\n",qcall[id].qt_call);
                        g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                        
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                    }
				}else if (CANCEL==method){
					printf("Receive CANCEL 200OK\n");
				}else if (method==REFER){
					printf("Receive REFER 202OK\n");
                    
                    char phonenumber[64]="0";
                    getPhoneToRsp(rsp,phonenumber);
                    QuStartTransferNotifyExpireThread(phonenumber);
                    
				}
			}else if((statuscode>=300)&&(statuscode<400)){
				//302 should use contact for new target
				if(statuscode==302){			
					char *newurl=GetRspContact(rsp);
					if(NULL!=newurl){
						//new request
						SipReq newreq=newReqFromOldReq(call,req,newurl,&callleg);
						//send request
						ccSendReq(&call,newreq,callleg);
						sipReqFree(newreq);
					}
				}
			}else if((statuscode>=400) &&(statuscode<600)){
			
				if (INVITE==method && statuscode!=401 && statuscode!=407){
					
					char *displayname=GetRspToDisplayname(rsp);
					if (!displayname){
						displayname=strdup("");
					}
					char phonenumber[64]="0";
					getPhoneToRsp(rsp,phonenumber);
					char remote_a_ip[32]="0";
					char remote_v_ip[32]="0";
					int raudio_port=0,rvideo_port=0;
					int callmode;
						
					int id=QuGetCall(phonenumber);
					
					int switch_mode;
					if (REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode){
						switch_mode = AUDIO;
					}else if (REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode){
						switch_mode = VIDEO;
					}
					printf("g_inviteProgress=FALSE\n");
					g_inviteProgress=FALSE;
					
					if ((REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode)||(REQUEST_SWITCH_TO_VIDEO==qcall[id].request_mode)){
	// 					if (statuscode==491){								
						//For all 4XX (406) case and the client is in switch mode, cancel the switch request

						printf("The switch request is cancelled.\n");					
						
						//!----For QT----//
                        if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                        {
                            // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                            pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                            printf("g_event_listner->onReinvteFailed %d\n",qcall[id].qt_call);
                            g_event_listner->onReinviteFailed(qcall[id].qt_call);
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                            pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                        }

						printf("qcall[id].request_mode=%d\n",qcall[id].request_mode);
						if (REQUEST_SWITCH_TO_AUDIO==qcall[id].request_mode){
							int *p_id = (int*) malloc( sizeof(int) );
							*p_id = id;
							pthread_t rei_thread;
							printf("switch to audio cancel , re-send invite with A/V %s\n",phonenumber);
							pthread_create(&rei_thread, NULL,reinvite_codec , (void*) p_id);
						}
						qcall[id].request_mode=REQUEST_EMPTY;
						
					}else if (REQUEST_HOLD==qcall[id].request_mode || REQUEST_UNHOLD==qcall[id].request_mode){
						printf("Re-invite receive error status code:%d\n",statuscode);
						if (statuscode==491){			

							if(qcall[id].callstate==HOLD_BEHOLD) {
							  printf("<sip> 491 leads HOLD_BEHOLD->BEHOLD\n");	
							  qcall[id].callstate = BEHOLD;
							}
							else if(qcall[id].callstate==HOLD) {
							  printf("<sip> 491 resume hold\n");
							  qcall[id].callstate = ACTIVE;
							}
						}
					}
					qcall[id].request_mode=REQUEST_EMPTY;
					qcall[id].wait_response=WAITRESPONSE_EMPTY;
					
                    if(g_event_listner != NULL && qcall[id].qt_call != NULL)
                    {
                        // Enter the critical section
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_lock(&qcall[id].qt_call->mCallData.calldata_lock);
                        
                        qcall[id].qt_call->mCallData.is_reinvite = false;
                        qcall[id].qt_call->mCallData.is_caller = false;
                        printf("g_event_listner->onCallStateChanged %d\n",qcall[id].qt_call);
                        g_event_listner->onCallStateChanged(qcall[id].qt_call, qcall[id].qt_call->mCallData.call_state, "");
                        
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","qcall[id].qt_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                        pthread_mutex_unlock(&qcall[id].qt_call->mCallData.calldata_lock);
                    }

					break;
				}
                
                if (BYE==method){
                	/*handle 4xx if Method is BYE*/
                    printf("receive BYE response !!status_code=%d\n!!!!",statuscode);
                    QuRevBye200OK(rsp);
                    break;
                }
                
				if (403==statuscode||404==statuscode||410==statuscode||413==statuscode||480==statuscode||486==statuscode||484==statuscode){
                    if(REGISTER==method){

                    	if (threadRegisterRetry!=-1){
							thrd_register_retry = false;
							pthread_join(threadRegisterRetry,NULL);
							threadRegisterRetry=-1;
						}
						reg.is_register = statuscode;
						pthread_create(&threadRegisterRetry, NULL, start_register_retry, NULL);

                        char msg[128];
                        snprintf(msg,128*sizeof(char), "voip account is unavailable(error code=%d)",statuscode);
                        QuRegisterFail(msg);
                    }
					
				}else{
					if (401 == statuscode || 407 == statuscode){
						
						if(REGISTER==method){
							SipReq newreq=NULL;
							char contact[256];
							char rount[256];
						
							//add realm fail, re-transmision time 
							//static int re_transmit=0;
							
							printf("\n%s\n",sipRspPrint(rsp));

							printf("401 reg.re_transmit=%d\n",reg.re_transmit);
							if (reg.re_transmit>=5){

								reg.re_transmit=0;
								//printf("401 reg[regid].re_transmit=%d\n",reg[regid].re_transmit);
								if (threadRegisterRetry!=-1){
									thrd_register_retry = false;
									pthread_join(threadRegisterRetry,NULL);
									threadRegisterRetry=-1;
								}
								reg.is_register = statuscode;
								pthread_create(&threadRegisterRetry, NULL, start_register_retry, NULL);
                                QuRegisterFail("account/password is not correct");
								return;
							}
							
							reg.re_transmit++;
							//printf("%d\n",re_transmit);
							g_register_progress = TRUE;

							if (reg.startkplive) {
                                thrd_reg = false;
                                pthread_join(reg.threadRegister,NULL);
                                sleep(1);
                                if (!b_itri_init) {
									return;
								}
                                reg.startkplive=false;
                            }
							
							//reg[regid].regreq=sipReqDup(req);
							//reg[regid].regrsp=sipRspDup(rsp);
							
							memcpy(&reg.regcall , &call ,sizeof(call));
								
							memset(&contact , 0, 256*sizeof(char));
							memset(&rount , 0, 256*sizeof(char));
				
							snprintf(contact,256*sizeof(char),"<sip:%s@%s:5060>",profile.sipid,profile.ip);
							snprintf(rount,256*sizeof(char),"<sip:%s:%d;lr>",profile.outbound_server,LOCALPORT);
							
							newreq=newReqFromOldReq(call,req,GetReqUrl(req),&callleg);
							
							ccReqAddAuthz(newreq,rsp,profile.username,profile.password);
							
							/*2010-08-10  duplicate authorization request*/
							//se.regreq=sipReqDup(newreq);
							
							if(newreq!=NULL){
								//send request
								printf("\n%s\n",sipReqPrint(newreq));

								ccSendReq(&call,newreq,callleg);
								sipReqFree(newreq);
							}	
							
							g_register_progress = FALSE;

						}else{
							
							pSipDlg dlg = NULL;
							ccMsgGetCallLeg(&postmsg,&dlg);
							
							if (PRACK==method ){
								printf("PRACK receive 401/407\n");
								if (dlg==NULL){
									printf("dlg==NULL\n");
								}else{
									printf("dlg!=NULL\n");
								}
								
								SipReq prackreq;
								if(ccNewReqFromCallLeg(&call,&prackreq,PRACK,NULL,NULL,&dlg)==RC_OK){
									printf("New PRACK for 401/407 ok\n");
									ccReqAddAuthz(prackreq,rsp,profile.username,profile.password);
									sipReqAddHdr(prackreq,RAck,sipReqGetHdr(req,RAck));
									
									ccSendReq(&call,prackreq,dlg);
									sipReqFree(prackreq);
								}
							}else{
								printf("other method receive 401/407\n");
								SipReq newreq=newReqFromOldReq(call,req,GetReqUrl(req),&dlg);
								ccReqAddAuthz(newreq,rsp,profile.username,profile.password);
							
								if(newreq!=NULL){
								//send request
								
								ccSendReq(&call,newreq,dlg);
								
								sipReqFree(newreq);
								}
							}		
						}
					}else {
						
						if (REGISTER==method){
							if (threadRegisterRetry!=-1){
								thrd_register_retry = false;
								pthread_join(threadRegisterRetry,NULL);
								threadRegisterRetry=-1;
							}
							reg.is_register = statuscode;
							pthread_create(&threadRegisterRetry, NULL, start_register_retry, NULL);
                            QuRegisterFail("voip server error");
						}else if (CANCEL==method){
							if (487 == statuscode){
								printf("!!!!CACNEL 487\n!!!!");
							}
						}else if (REFER==method){
							printf("REFER!!!status_code=%d\n!!!!",statuscode);
						}
					}
				}
			}else if(statuscode>=600){
				/*6xx will wait interval time and try primary server*/
				
				if (REGISTER==method){
					if (threadRegisterRetry!=-1){
						thrd_register_retry = false;
						pthread_join(threadRegisterRetry,NULL);
						threadRegisterRetry=-1;
					}
					reg.is_register = statuscode;
					pthread_create(&threadRegisterRetry, NULL, start_register_retry, NULL);
					QuRegisterFail("voip server error");
				}
				
			}
			break;
		case SIP_CANCEL_EVENT:
			/* send 487 and drop another call */
			printf("receive cancel\n");
			getPhoneFromReq(req,phonenumber);
			id = QuGetCall(phonenumber);
			
			if(qcall[id].response_mode==RESPONSE_SWITCH_TO_VIDEO || qcall[id].response_mode==RESPONSE_SWITCH_TO_AUDIO) {
			  ccRejectCall(&call,406,"Not Acceptable",NULL);
			} else {
			  ccRejectCall(&call,487,"call cancelled",NULL);
			}

			QuRevCancel(req);

			break;
		//case SIP_NO_MATCH_REQ:
			//break;
		case SIP_TX_TIMEOUT:
            method=GetReqMethod(req);
			if (method==BYE){
				printf("BYE timeout\n");
				QuByeTimeout(req);
			}else if (method==INVITE){
				printf("INVITE timeout\n");
				QuInviteTimeout(req);
			}else if (method==CANCEL){
				printf("CANCEL timeout\n");
				char phonenumber[64]="0";
				getPhoneToReq(req,phonenumber);
	
				int id = QuGetCall(phonenumber);				
				QuCancelTimeout(id);
			}else if (method == REGISTER)
            {
                printf("REGISTER timeout\n");
                if (threadRegisterRetry!=-1){
                	thrd_register_retry = false;
                	pthread_join(threadRegisterRetry,NULL);
                	threadRegisterRetry=-1;
                }

                reg.is_register = -1;

                pthread_create(&threadRegisterRetry, NULL, start_register_retry, NULL);
                QuRegisterFail("voip request timeout");
            }
			break;
		case SIP_TX_ERR:
            printf("SIP_TX_ERR\n");
			break;
		case SIP_TX_ETIME:
            printf("SIP_TX_ETIME\n");
			break;
	}
}

void *thread_function_Events(void *arg) {

	/*
	 *	run dialog layer event dispatch
	 */
	thrd_event = TRUE;
	printf("start thread_function_Events,value=%d\n",thrd_event);
	while(thrd_event){
		CallCtrlDispatch(100);
		//usleep(1);
	}
	printf("end thread_function_Events\n");
	pthread_exit(0);
	
	return NULL;
}

void audio_codec_init(){
	
	strcpy(audio_codec[PCMU].payload,"0");
	strcpy(audio_codec[PCMU].codec,"PCMU/8000");
	strcpy(audio_codec[PCMU].sample_rate,"8000");
	
	
	//audio_codec[G7221].payload="121";
	char g7221_codec[32]="0";
	char g7221_payloadtype[32]="102";
	char g7221_samplerate[32]="16000";
	char g7221_bitrate[32]="32000";
	
	
	strcpy(audio_codec[G7221].payload,g7221_payloadtype);
	snprintf(g7221_codec,32*sizeof(char),"G7221/%s",g7221_samplerate);
	strcpy(audio_codec[G7221].codec,g7221_codec);
	strcpy(audio_codec[G7221].sample_rate,g7221_samplerate);
	strcpy(audio_codec[G7221].fmap,g7221_bitrate);

	strcpy(audio_codec[PCMA].payload,"8");
	strcpy(audio_codec[PCMA].codec,"PCMA/8000");
	strcpy(audio_codec[PCMA].sample_rate,"8000");

	strcpy(audio_codec[G722].payload,"9");
	strcpy(audio_codec[G722].codec,"G722/8000");
	strcpy(audio_codec[G722].sample_rate,"8000");

	/*
	 * codec OPUS
	 */
	strcpy(audio_codec[OPUS].payload,"111");
	strcpy(audio_codec[OPUS].codec,"OPUS/48000/2");
	strcpy(audio_codec[OPUS].sample_rate,"48000");
}

int itri_init(const char* localIP,unsigned int listenPort){

	SipConfigData ConfigStr;
	char rurl[256];
		
	/*
	*	initial library and set config data
	*/
	
    setbuf(stdout, NULL);
    
	ConfigStr.tcp_port = listenPort;
    //ConfigStr.tcp_port = 5060;
	#if CALLCONTROL_LOG 
		ConfigStr.log_flag = LOG_FLAG_CONSOLE;
	#else	
		ConfigStr.log_flag = LOG_FLAG_NONE ;//no trace log
	#endif

	#if CALLCONTROL_LOG 
		strcpy(ConfigStr.fname,"./callctrl.log");
		ConfigStr.trace_level = TRACE_LEVEL_API;
	#else
		ConfigStr.trace_level = TRACE_LEVEL_NONE ;//print nothing
	#endif

	
	memset(&qcall, 0, sizeof(qcall));//Initiate struct QuCall
	memset(&profile, 0, sizeof(profile));//Initiate struct QuProfile
	memset(&reg, 0, sizeof(reg));//Initiate struct QuProfile
	
	strcpy(profile.ip, localIP); //quanta

	audio_codec_init();

	ConfigStr.rport = 5060;
	
	/*TLS Configutation*/
	ConfigStr.tls_port=0;
	strcpy(ConfigStr.caCert,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-cert.pem");
	strcpy(ConfigStr.caKey,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-privkey.pem");
	strcpy(ConfigStr.caKeyPwd,"opensips");
	strcpy(ConfigStr.caPath,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-calist.pem");
	if (!strcmp("1",sip_config.generic.sip.tls_enable)){
		ConfigStr.enableTLS='Y';
	}else{
		ConfigStr.enableTLS='N';
	}

    ConfigStr.UseShortFormHeader=FALSE;
	strcpy(ConfigStr.laddr,profile.ip);
	printf("ConfigStr.laddr=%s\n",ConfigStr.laddr);

	rCode=CallCtrlInit(EvtCB,ConfigStr,"");
	
	if(rCode!=RC_OK){
		printf("call control initiation is failed\n");
		return -1;
	}else{
		printf("call control initiated\n");
	}

	memset(rurl,0,256*sizeof(char));
	snprintf(rurl,256*sizeof(char),"sip:%s:%d",profile.proxy_server,listenPort);

	//media_bandwidth = atoi(H264_CT_BANDWIDTH);
	media_bandwidth = local_video_mdp.tias_kbps;
	
#ifdef _WIN32
	snprintf(g_product,80*sizeof(char),"%s %s_1.0.1",COMPANY,PROJECT_QT1W);
#endif

#ifdef _ANDROID_APP
	snprintf(g_product,80*sizeof(char),"%s %s_1.0.1",COMPANY,PROJECT_QT1A);
#endif
    
#ifndef _ANDROID_APP
	snprintf(g_product,80*sizeof(char),"%s %s_1.0.1",COMPANY,PROJECT_QT1I);
#endif
    
	/*start event dispatchr*/	
	//pthread_t threadGeteXEvents;
	int intRes;
	intRes = pthread_create(&threadGeteXEvents, NULL, thread_function_Events, NULL);
	if (intRes != 0) {
		perror("Thread creation failed!");
		exit(EXIT_FAILURE);	
	}	

	g_local_listen_port = listenPort;

	if (!strcmp("1",sip_config.generic.rtp.srtp_enable)){
		enable_srtp = 1;
	}else{
		enable_srtp = 0;
	}

    printf("[INFO]sip local listen port=%d\n",g_local_listen_port);
	printf("[INFO]default value=%s\n",sip_config.qtalk.sip.audio_codec_preference_1);

	return 1;
}

void *task(void *arg){
    
    printf("!!TASK!!!\n");
    
    sleep(3);
    
    //NetworkInterfaceSwitch("192.168.199.126",5060);
    sip_unattend_transfer("18012", "12301");
    
    return NULL;
}

pthread_mutex_t interfaceSwitch_lock;

void cleanCall()
{

	int i = 0;

	g_inviteProgress=FALSE;

	for (i=0;i<NUMOFCALL;i++) {
	   if (qcall[i].call_obj != NULL) {
		   printf("callstat=%d, phone=%s\n",qcall[i].callstate,qcall[i].phone);
		   ccDropCall(&qcall[i].call_obj,NULL);
		   QuFreeCall(i,false,"");
	   }
   }
}

void cleanRegisterRetry()
{

	if (reg.startkplive) {
		thrd_reg = false;
		pthread_join(reg.threadRegister,NULL);
	    reg.startkplive=false;
	}

	if (threadRegisterRetry!=-1){
		thrd_register_retry = false;
		pthread_join(threadRegisterRetry,NULL);
		threadRegisterRetry=-1;
	}
}

//
void cleanAllCallObject()
{
     g_inviteProgress=FALSE;
    
    if (reg.startkplive) {
        thrd_reg = false;
        pthread_join(reg.threadRegister,NULL);
        reg.startkplive=false;
    }
    
    if(threadGeteXEvents!=NULL)
    {
		printf("threadGeteXEvents!=NULL\n");
		thrd_event=false;
		pthread_join(threadGeteXEvents, NULL);
		printf("join thread GeteX Events\n");
		threadGeteXEvents = NULL;
    }

    sleep(2);
    
    if (threadRegisterRetry!=-1){
    	printf("threadRegisterRetry!=NULL\n");
    	thrd_register_retry = false;
    	pthread_join(threadRegisterRetry,NULL);
    	printf("join threadRegisterRetry\n");
    	threadRegisterRetry=-1;
    }

    if (reg.regcall!=NULL){
        ccCallFree(&reg.regcall);
        reg.regcall=NULL;
    }
    
    if (reg.user!=NULL){
        ccUserFree(&reg.user);
        reg.user=NULL;
    }
    
    if (reg.regreq){
        sipReqFree(reg.regreq);
        reg.regreq=NULL;
    }
    
    if (reg.regrsp){
        sipRspFree(reg.regrsp);
        reg.regrsp=NULL;
    }
    
    

}

int QuRegisterFail(const char* msg)
{
    printf("[WARNING]=QuRegisterFail, reason=%s\n",msg);
    if (g_event_listner !=NULL){
    	g_event_listner->onLogonState(false, msg, 0, profile.ip);
    }
    return 1;
}


void NetworkInterfaceSwitch(const char* localIP,unsigned int listenPort ,int Network_Interface_Type){
    
    printf("!!!!!NetworkInterfaceSwitch\n");
    if (Network_Interface_Type==NETWORK_NONE) {
		printf("Network_Interface_Type = %d g_network_inetrface_type = %d return",Network_Interface_Type, g_network_inetrface_type);
		g_network_inetrface_type = Network_Interface_Type;
		return;
	}
    
    if(Network_Interface_Type==g_network_inetrface_type
           && !strcmp(profile.ip, localIP)){
		printf("Same interface and same IP, register again only\n");

		if (reg.startkplive) {
			/*stop keep-alive thread*/
			thrd_reg = false;
			pthread_join(reg.threadRegister,NULL);
			reg.startkplive=false;
		}
		sip_make_registration(profile.proxy_server, profile.outbound_server, profile.realm,
				profile.sipid, profile.username, profile.password, true);
		return;
	}

#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","interfaceSwitch_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    pthread_mutex_lock(&interfaceSwitch_lock);
    
    cleanAllCallObject();
 
    int i=0;
    
    for (i=0;i<NUMOFCALL;i++) {
        if (qcall[i].call_obj!=NULL) {
        	if (qcall[i].sip_terminate_call){
        		printf("had sip_terminate_call ,QuFreeCall i=%d\n",i);
        		QuFreeCall(i,false,"");
        	}else{
        		printf("ccGetCallFromDB i=%d\n",i);
        		ccGetCallFromDB(&qcall[i].call_obj);
        		//check call statse
        		ccCallState cs;
        		ccCallGetCallState(&qcall[i].call_obj,&cs);

				for(int retryTime = 0; retryTime < 5 ;retryTime ++) {
					if(cs >= CALL_STATE_PROCEED) break;

					printf("\n\n\n!!!!!cs<CALL_STATE_PROCEED\n\n\n");
					sleep(1);
					ccCallGetCallState(&qcall[i].call_obj,&cs);
				}

        	}
        }
    }
    printf("before CallCtrlClean()\n");

	rCode=CallCtrlClean();
	if (rCode==RC_OK){
		printf("CallCtrlClean ok\n");
	}else{
		printf("CallCtrlClean failed\n");
	}

	sleep(1);
	
	SipConfigData ConfigStr;
	
	/*
     *	initial library and set config data
     */
	
	g_local_listen_port = listenPort;
	ConfigStr.tcp_port = g_local_listen_port;
	printf("g_local_listen_port=%d\n",g_local_listen_port);

#if CALLCONTROL_LOG 
    ConfigStr.log_flag = LOG_FLAG_CONSOLE;
#else	
    ConfigStr.log_flag = LOG_FLAG_NONE ;//no trace log
#endif
    
#if CALLCONTROL_LOG 
    strcpy(ConfigStr.fname,"./callctrl.log");
    ConfigStr.trace_level = TRACE_LEVEL_API;
#else
    ConfigStr.trace_level = TRACE_LEVEL_NONE ;//print nothing
#endif
	
	
    /*TLS Configutation*/
	ConfigStr.tls_port=0;
	strcpy(ConfigStr.caCert,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-cert.pem");
	strcpy(ConfigStr.caKey,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-privkey.pem");
	strcpy(ConfigStr.caKeyPwd,"opensips");
	strcpy(ConfigStr.caPath,"/home/nicky/workshop/QRI/sip/iclsip-rmu131_tls/user-calist.pem");
	if (!strcmp("1",sip_config.generic.sip.tls_enable)){
		ConfigStr.enableTLS='Y';
	}else{
		ConfigStr.enableTLS='N';
	}

	ConfigStr.rport = 5060 ;
	ConfigStr.UseShortFormHeader=FALSE;
	sleep(1);
	
	strcpy(profile.ip, localIP); //quanta
	
    strcpy(ConfigStr.laddr,profile.ip);
	printf("ConfigStr.laddr=%s\n",ConfigStr.laddr);
    
	rCode=CallCtrlInit(EvtCB,ConfigStr,"");
    
	if (rCode==RC_OK)
    {
		printf("CallCtrlInit ok\n");
        /*start event dispatch*/
        //pthread_t threadGeteXEvents;
        int intRes;
        intRes = pthread_create(&threadGeteXEvents, NULL, thread_function_Events, NULL);
        if (intRes != 0) {
            perror("Thread creation failed!");
            return;
        }
    }
	else
    {
        printf("CallCtrlInit fail, fatal error\n");
        //assert(0&&"CallCtrlInit fail, fatal error");
        return;
    }
    
    for (i=0;i<NUMOFCALL;i++) {
        if (qcall[i].call_obj!=NULL) {
            printf("ccSetCallToDB , i=%d\n",i);
            ccSetCallToDB(&qcall[i].call_obj);
        }
    }
    
        
    sip_make_registration(profile.proxy_server, profile.outbound_server, profile.realm, profile.sipid
    		, profile.username, profile.password, true);
    
    /*
	char usercontact[256];
	memset(usercontact,0,sizeof(usercontact));
	sprintf(usercontact,"<sip:%s@%s:5060>",profile.sipid,localIP);
	ccUserSetContact(&reg.user,usercontact);
    */
    
    for (i=0;i<NUMOFCALL;i++) {
        if (qcall[i].call_obj!=NULL) {
            printf("ccCallSetUserInfo , i=%d\n",i);
            ccCallSetUserInfo(&qcall[i].call_obj,reg.user);
        }
    }
    
    for (i=0;i<NUMOFCALL;i++) {
        if (qcall[i].call_obj!=NULL) {
           printf("reinvite_codec , i=%d\n",i);
            int *p_id = (int*) malloc( sizeof(int) );
            *p_id = i;
            
            //qcall[i].network_inetrface_type = Network_Interface_Type;
            
            pthread_t rei_thread[NUMOFCALL];
            printf("newwork interface chabge , send re-invite to change ip , id=%d\n",i);
            pthread_create(&rei_thread[i], NULL,reinvite_codec , (void*) p_id);
            pthread_join(rei_thread[i], NULL);
        }
    }
    
    g_network_inetrface_type=Network_Interface_Type;
    
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","interfaceSwitch_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    pthread_mutex_unlock(&interfaceSwitch_lock);
}
