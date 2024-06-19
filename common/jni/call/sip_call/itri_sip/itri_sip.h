#ifndef ITRI_SIP_H
#define ITRI_SIP_H
#include <stdio.h>

#include "icall_engine.h"
#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif
#include <sipTx/sipTx.h>

#include "icall_engine.h"
#include "itri_call.h"

namespace itricall{class ItriCall;};

#define SLEN 255
#define ADDRESS_LEN 32
struct list {
	int no;
	int port;
	char name[SLEN];
	char address[ADDRESS_LEN];
	struct list *next;
};

int itri_init(const char* localIP,unsigned int listenPort);
void EvtCB(CallEvtType event, void* msg);

int sip_make_registration(const char* proxysv,const char* outboundsvr,const char* realm,const char* sipid
		,const char* name,const char* psw, bool isLogin);

int sip_make_call(itricall::ItriCall* call, unsigned int mode, char* ruuid, char* addr, int audio_port , int video_port);

int sip_answer_call(itricall::ItriCall* call,int mode, char* addr);
int sip_terminate_call(char* addr);
int sip_terminate_call_by_id(int callid);
int sip_cancel_call(char* addr);
int sip_cancel_call_by_id(int callid);
int sip_reject_call(char* addr);
int sip_reject_call_by_id(int callid);
int sip_hold_call(char* addr);
int sip_resume_call(char* addr);

void sip_unattend_transfer(char* addr, char* target_addr);
int sip_transfer_terminate_call(char* addr);

int sip_request_switch(int mode, char* addr);
int sip_confirm_switch(int accept, int mode, char* addr);

void sip_request_idr(int call_id);

void set_profile(char* proxysv, char* outboundsvr, char* realm, char* sipid ,char* name, char* psw , char* local_ip);
void set_event_listner( call::ICallEventListener* listener);
void *thread_function_Events(void *arg);

void *task(void *arg);
void NetworkInterfaceSwitch(const char* localIP,unsigned int listenPort , int Network_Interface_Type);

void cleanAllCallObject();
void cleanRegisterRetry();
void cleanCall();
//ItriCall* findInternalCall(const ItriCall* pcall);

#endif
