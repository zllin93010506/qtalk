/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_call.c
 *
 * $Id: callctrl_call.c,v 1.11 2009/02/23 07:12:38 tyhuang Exp $
 */
/** \file callctrl_call.c
 *  \brief Define ccCall struct access functions
 *  \author Huang,Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#include <stdio.h>
#include <adt/dx_lst.h>
#include <adt/dx_hash.h>
#include <sipTx/TxStruct.h>
#include <sipTx/sipTx.h>
#include <DlgLayer/dlglayer.h>

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

#include "callctrl.h"

DxHash	CallDBLst=NULL;
CallEvtCB g_callevtcb=NULL;
char	*g_cclocalIPAddress=NULL;
UINT32	g_cclistenport=5060;
char	*g_ccrealm=NULL;
char	*g_remoteDisplayname=NULL;

struct ccCallObj{
	char			*callid;
	UINT32			registcseq;
	ccCallState		callstate;
	char			*laddr;
	pCCUser			usersetting;
	DxLst			dlglist;
	DxHash			txdb;
	void			*userdata; /* for user of callctrl layer */

	/* for outbound proxy setting in sipTx */
	UserProfile		outboundproxy;

	/* for internal use */
	pSipDlg			inviteDlg;
};

RCODE ccCallSetCallID(IN  pCCCall *const,IN char *);
RCODE CallCtrlPostMsg(IN CallEvtType ,IN pCCMsg);

pSipDlg matchDialogByRsp(IN pCCCall,IN SipRsp,IN INT32);
SipReq GetReqFromCall(IN pSipDlg);
RCODE getUserUrl(IN pCCCall call,OUT char* buffer,IN OUT UINT32* length);
const char* callstateToStr(IN ccCallState cs);

BOOL CheckContParam(IN SipReq );


pCCCall	MatchCall(IN char *callid)
{
	pCCCall call=NULL;
	Entry calllist;

	calllist=dxHashItemLst(CallDBLst,callid);
	while(calllist){
		/* match by callid */
		if (strCmp(dxEntryGetKey(calllist),callid)==0) {
			call=(pCCCall)dxEntryGetValue(calllist);
			break;
		}
		calllist=dxEntryGetNext(calllist);
	}
	return call;
}

BOOL IsDlgExist(IN pCCCall call,pSipDlg dlg)
{
	INT32 pos,size;

	if (!call) {
		return FALSE;
	}
	if (!dlg) {
		return FALSE;
	}

	size=dxLstGetSize(call->dlglist);
	for(pos=0;pos<size;pos++){
		if (dxLstPeek(call->dlglist,pos)==dlg) 
			return TRUE;
	}
	return FALSE;
}

pCCCall	MatchCallByDlg(IN pSipDlg dlg)
{
	INT32 len=255;
	char key[256];
	Entry calllist;
	pCCCall call=NULL;

	if (RC_OK!=sipDlgGetCallID(&dlg,key,&len)) {
		CCWARN("[MatchCallByDlg] call is NULL!\n");
		return NULL;
	}
	calllist=dxHashItemLst(CallDBLst,key);
	while(calllist){
		call=(pCCCall)dxEntryGetValue(calllist);
		if(IsDlgExist(call,dlg)==TRUE)
			break;
		else
			call=NULL;
		calllist=dxEntryGetNext(calllist);
	}

	return call;
}


BOOL IsEmpty(const char *s)
{

	if(!s || strlen(s)==0)
		return TRUE;
	else
		return FALSE;
}

void FreeUserData(UserData *ud)
{

	if(!ud)
		return;

	free(ud);
}

RCODE AddDLGtoCALL(pCCCall call,pSipDlg dlg)
{
	INT32 size,pos;
	
	if( !call || !dlg )
		return RC_ERROR;

	/* create list if it doesnt exist */
	if(call->dlglist==NULL)
		call->dlglist=dxLstNew(DX_LST_POINTER);

	size=dxLstGetSize(call->dlglist);
	for(pos=0;pos<size;pos++)
		if(dxLstPeek(call->dlglist,pos)==dlg)
			return RC_OK;
	return dxLstPutTail(call->dlglist,dlg);
}

RCODE postprocessex(pCCCall call,SipReq req,SipRsp rsp,CallEvtType postevent,pSipDlg dlg,pCCCall rcall)
{
	pCCMsg postMsg=NULL;

	if(!call)
		return RC_ERROR;
	if(!req&&!rsp)
		return RC_ERROR;

	if(postevent == CALL_STATE)
		CCPrintEx ( TRACE_LEVEL_API, "[postprocess] enter to post call state change!\n" );
	
	if(ccMsgNew(&postMsg,call,req,rsp,dlg)==RC_OK){
		if(rcall!=NULL)
			ccMsgSetReplacedCall(&postMsg,rcall);

		if(CallCtrlPostMsg(postevent,postMsg)!=RC_OK){
			CCWARN("[callreqprocess] post msg is failed!\n" );	
			return RC_ERROR;
		}
		ccMsgFree(&postMsg);
	}
	return RC_OK;
}

RCODE postprocess(pCCCall call,SipReq req,SipRsp rsp,CallEvtType postevent,pSipDlg dlg)
{
	return postprocessex(call,req,rsp,postevent,dlg,NULL);
}


RCODE callreqprocess(IN pDlgMsg* pMsg,IN SipReq req)
{
	void	*handle=NULL;
	pSipDlg dlg=NULL;
	pCCCall call=NULL;
	char	*callid,buffer[256];
	pDlgMsg Msg=NULL;
	INT32 buflen=255;
	UserData *txuser;
	CallEvtType postevent=SIP_REQ_REV;
	SipMethodType method;
	
	pSipDlg rdlg=NULL;

	/*
	SipRsp rsp;
	SipCSeq *reqcseq;
	*/

	if( !pMsg ){
		CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	
	Msg=*pMsg;
	if( !Msg ){
		CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if( !req ) {
		CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}

	method=GetReqMethod(req);
	sipDlgMsgGetDlg(&Msg,&dlg);

	if(!dlg){
		/* no-dialog request */
		sipDlgMsgGetHandle(&Msg,&handle);
		if(!handle){
			CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Get a Bad message!\n" );	
			return RC_ERROR;
		}else{
			if(NULL==GetReqViaBranch(req)){
				CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Get a Bad request,via brach id is null!\n" );	
				return RC_ERROR;
			}
			/* set tx userdata and add tx to call */
			callid=(char*)sipTxGetCallid((TxStruct)handle);
			if( !callid ) {
				CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Get a Bad request,callid is null!\n" );	
				return RC_ERROR;
			}
			call=MatchCall(callid);
			if( !call ){
				/* create a call object for it */
				if(RC_OK != ccCallNewEx(&call,callid)){
					CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] New call is failed!\n" );	
					return RC_ERROR;
				}
				/* set call id from request 
				ccCallSetCallID(&call,callid);*/
			}

			/* set userdata for handle */
			if(sipTxGetUserData((TxStruct)handle)==NULL){
				txuser=(UserData*)calloc(1,sizeof(UserData));
				if( !txuser ){
					CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Invalid argument : Memory allocation error\n" );
					return RC_ERROR;
				}
				txuser->usertype=txu_call;
				txuser->data=(void*)call;
				sipTxSetUserData((TxStruct)handle,(void*)txuser);
			}
		
			if (RC_OK!=dxHashAddEx(call->txdb,GetReqViaBranch(req),(void*)handle)) 
				CCWARN("[callreqprocess] add tx to call is failed!\n" );
			CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Get an out of dialog request message!\n" );
			
		}
	}else{
		/* get a dialog */
		if(sipDlgGetCallID(&dlg,buffer,&buflen)==RC_OK){
			call=MatchCallByDlg(dlg);
			/* check method if invite ,create a new call for it */
			if( !call ){
				/* create a call object for it */
				if(RC_OK != ccCallNew(&call)){
					CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] New call is failed!\n" );	
					return RC_ERROR;
				}
				/* set call id from request */
				ccCallSetCallID(&call,buffer);
			}
			/* set userdata for dialog */
			sipDlgSetUserData(&dlg,(void*)call);
			
			/* add dlg to call */
			if(RC_OK!=AddDLGtoCALL(call,dlg))
				CCWARN("[callreqprocess] add dialog to call is failed!\n" );	

			CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Get a dialog request message!\n" );	
		}else{
			CCWARN("[callreqprocess] Get a CallID for dialog is failed!\n" );	
			return RC_ERROR;
		}
	}
	
	/* modified call state if method is Invite */
	if(method==INVITE){
		if((call->callstate==CALL_STATE_DISCONNECT)||(call->callstate==CALL_STATE_NULL)){
			call->callstate=CALL_STATE_OFFER;
			call->inviteDlg=dlg;
			postevent=CALL_STATE;
		}
	}else if(method==ACK){
		if(call->callstate==CALL_STATE_ANSWER){
			call->callstate=CALL_STATE_CONNECT;
			postevent=CALL_STATE;
		}
	}else if(method==BYE){
		if((call->callstate==CALL_STATE_CONNECT)
			||(call->callstate==CALL_STATE_ANSWER)
			||(call->callstate==CALL_STATE_ANSWERED)){
			
			call->callstate=CALL_STATE_DISCONNECT;
			postevent=CALL_STATE;
		}
	}

	if(postevent==CALL_STATE){
		CCPrintEx1(TRACE_LEVEL_API,"[callreqprocess] post call id is %s \n",call->callid);
		CCPrintEx1(TRACE_LEVEL_API,"[callreqprocess] call state is %s \n",callstateToStr(call->callstate));
	}
	
	/* check content parameter */
	if(INVITE==method){
		if(!CheckContParam(req)){
			ccRejectCall(&call,415,"Unsupported Media",NULL);
			return RC_ERROR;
		}

		if(!CheckOptionTag(req)){
			SipRsp unrsp=sipRspNew();

			sipRspAddHdr(unrsp,Unsupported,sipReqGetHdr(req,Require));
			ccRejectCallEx(&call,420,"Bad Extension",NULL,unrsp);
			sipRspFree(unrsp);
			return RC_ERROR;
		}
	}
	
	/* check Require parameter */
	sipDlgMsgGetReplacedDlg(pMsg,&rdlg);
	if(NULL==rdlg){
		/* post message */
		if(postprocess(call,req,NULL,postevent,dlg)!=RC_OK)
			CCERR("[callreqprocess] post msg is failed!\n");
	}else{
		/* post message */
		void *rud=NULL;
		sipDlgGetUserData(&rdlg,&rud);
		if(postprocessex(call,req,NULL,postevent,dlg,(pCCCall)rud)!=RC_OK)
			CCERR("[callreqprocess] post msg is failed!\n");
	}
		
	
	return RC_OK;
}

RCODE callrspprocess(IN pDlgMsg* pMsg,IN SipRsp rsp,IN SipReq req)
{
	pSipDlg dlg=NULL;
	pCCCall call=NULL;
	char	*callid,buffer[256];
	pDlgMsg Msg=NULL;
	INT32 buflen=255;
	UserData *txuser=NULL;
	void	*handle=NULL,*dlgud=NULL;
	CallEvtType postevent=SIP_RSP_REV;
	SipMethodType method;
	INT32 statuscode;

	if( !pMsg ){
		CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	
	Msg=*pMsg;
	if( !Msg ){
		CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if( !rsp ) {
		CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] Invalid argument : Null pointer to response\n" );
		return RC_ERROR;
	}
	sipDlgMsgGetDlg(&Msg,&dlg);

	if(!dlg){
		/* no-dialog request */
		sipDlgMsgGetHandle(&Msg,&handle);
		if(!handle){
			CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] Get a Bad message!\n" );	
			return RC_ERROR;
		}else{
			/* set tx userdata and add tx to call */
			txuser=(UserData*)sipTxGetUserData((TxStruct)handle);
			if(txuser){
				if(txuser->usertype==txu_call)
					call=(pCCCall)txuser->data;
			}
			callid=(char*)sipTxGetCallid((TxStruct)handle);
			if( !callid ) {
				CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] Get a Bad request,callid is null!\n" );	
				return RC_ERROR;
			}
			if( !callid ){
				call=MatchCall(callid);
				if( !call ){
					CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] callid is dismatch!\n" );	
					return RC_ERROR;
				}
			}
		}
	}else{
		/* get a dialog */
		if(sipDlgGetUserData(&dlg,&dlgud)!=RC_OK){
			CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] userdata of dialog is null!\n" );	
			if(sipDlgGetCallID(&dlg,buffer,&buflen)==RC_OK)
				call=MatchCall(buffer);
		}else
			call=(pCCCall)dlgud;
		if( !call ){
			CCPrintEx ( TRACE_LEVEL_API, "[callrspprocess] callid is dismatch!\n" );	
			return RC_ERROR;
		}
	}
	
	/* modified call state if method is Invite */
	method=GetRspMethod(rsp);
	statuscode=GetRspStatusCode(rsp);
	if(method==INVITE){
		if(statuscode==100){
			if(call->callstate==CALL_STATE_INITIAL){
				call->callstate=CALL_STATE_PROCEED;
				postevent=CALL_STATE;
			}
		}else if(statuscode==180){
			if(call->callstate<CALL_STATE_CONNECT){
				call->callstate=CALL_STATE_RINGING;
				postevent=CALL_STATE;
			}
		}else if((statuscode>=200)&&(statuscode<300)){
			if(call->callstate<CALL_STATE_ANSWERED){
				call->callstate=CALL_STATE_ANSWERED;
				postevent=CALL_STATE;
			}
		}else if(statuscode>=300){
			/* will handle 302 or 305 */
			if(call->callstate<CALL_STATE_ANSWERED){
				call->callstate=CALL_STATE_DISCONNECT;
				postevent=CALL_STATE;
			}
			if(call->callstate == CALL_STATE_CONNECT){
				/*if((statuscode == 408)||( statuscode ==481)){*/
				if( statuscode ==481 ){
					call->callstate=CALL_STATE_DISCONNECT;
					postevent=CALL_STATE;
				}
			}
		}
	}else if( method== BYE){
		if((statuscode>=200)&&(statuscode<300)){
			if((call->callstate==CALL_STATE_ANSWER)||(call->callstate==CALL_STATE_ANSWERED)||(call->callstate==CALL_STATE_CONNECT)){
				call->callstate=CALL_STATE_DISCONNECT;
				postevent=CALL_STATE;
			}
		}
	}
	
	/* check dialog state if it's DLG_STATE_TERMINATED
	 * and dlg is invitedlg,then set call disconnect
	 */
	if((dlg==call->inviteDlg)&&(dlg!=NULL)&&(call->callstate!=CALL_STATE_DISCONNECT)){
		/*
		switch(statuscode) {
			case 404:
			case 408:
			case 480:
			case 481:
			case 483:
			case 487:
				call->callstate=CALL_STATE_DISCONNECT;
				postevent=CALL_STATE;
				break;
			default:
				break;
			}
		*/
		if(beSipDlgTerminated(&dlg)){
			call->callstate=CALL_STATE_DISCONNECT;
			postevent=CALL_STATE;
		}
	}
	
	/* post message */
	if(postprocess(call,req,rsp,postevent,dlg)!=RC_OK)
		CCERR("[callrspprocess] post msg is failed!\n");
	return RC_OK;
}

RCODE handleTransErr(DlgEvtType event,pDlgMsg msg)
{
	CallEvtType postevent=SIP_TX_ERR;
	UserData *ud=NULL;
	void *handle,*dlgud=NULL;
	TxStruct tx;
	pCCCall call=NULL;
	pSipDlg	dlg=NULL;
	SipReq	req=NULL;
	SipRsp	rsp=NULL;

	sipDlgMsgGetHandle(&msg,&handle);
	if(!handle){
		CCERR("[handleTransErr]get a wrong dialog message\n");
		return RC_ERROR;
	}
	tx=(TxStruct)handle;
	sipDlgMsgGetRsp(&msg,&rsp);
	if(!rsp)
		sipDlgMsgGetReq(&msg,&req);

	if( !rsp && !req )
		return RC_ERROR;
	
	sipDlgMsgGetDlg(&msg,&dlg);
	if (event==DLG_2XX_EXPIRE) {
		/* not receive ack,so send Bye to disconnect call */
		if (dlg) {
			sipDlgGetUserData(&dlg,&dlgud);
			call=(pCCCall)dlgud;
			if(call){
				ccDropCall(&call,NULL);
				return RC_OK;
			}else
				CCERR("[handleTransErr]call object is not found\n");
		}else
			CCERR("[handleTransErr]dlg is not found\n");
		return RC_ERROR;
	}
	if(dlg){/* in-dialog request fail */
		sipDlgGetUserData(&dlg,&dlgud);
		call=(pCCCall)dlgud;
		if(call&&(call->inviteDlg==dlg)){
			postevent=CALL_STATE;
			call->callstate=CALL_STATE_DISCONNECT;
		}
	}else{
		ud=(UserData*)sipTxGetUserData(tx);
		if(ud){
			if(ud->usertype==txu_call)
				call=(pCCCall)ud->data;
		}
	}
	
	if (DLG_MSG_TX_TIMEOUT==event){ 
		postevent=SIP_TX_TIMEOUT;
		rsp=NULL;
	}
	/* post message */
	if(postprocess(call,req,rsp,postevent,dlg)!=RC_OK)
		CCERR("[handleTransErr] post msg is failed!\n");

	return RC_OK;
}

RCODE cancelprocess(pDlgMsg msg)
{
	void *dlgud=NULL;
	pCCCall call=NULL;
	pSipDlg	dlg=NULL;
	SipReq	req=NULL;

	sipDlgMsgGetDlg(&msg,&dlg);
	if(!dlg){
		CCERR("[cancelprocess]get a wrong dialog message\n");
		return RC_ERROR;
	}

	sipDlgGetUserData(&dlg,&dlgud);
	call=(pCCCall)dlgud;
	sipDlgMsgGetReq(&msg,&req);
	if (!call || ! req) {
		CCERR("[cancelprocess]call object or original request is not found!\n");
		return RC_ERROR;
	}
	/* post message */
	if(postprocess(call,req,NULL,SIP_CANCEL_EVENT,dlg)!=RC_OK)
		CCERR("[cancelprocess] post msg is failed!\n");
	return RC_OK;
}

void DlglayEvtCB(DlgEvtType event, void* msg)
{
	pDlgMsg	testmsg=(pDlgMsg)msg;
	SipReq	req=NULL;
	SipRsp	rsp=NULL;

	if(!testmsg)
		return;

	switch( event) {
	case DLG_MSG_REQ:
		/* get Request from posted message */
		if(sipDlgMsgGetReq(&testmsg,&req)==RC_OK){
			CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]get a request!\n");
			if(callreqprocess(&testmsg,req)!=RC_OK)
				CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]process request is failed!\n");
		}
		break;
	case DLG_MSG_RSP:
		/* get response from posted message */
		if(sipDlgMsgGetRsp(&testmsg,&rsp)==RC_OK){
			CCPrintEx ( TRACE_LEVEL_API,"[DlglayEvtCB]get response\n");
			sipDlgMsgGetReq(&testmsg,&req);
			if(callrspprocess(&testmsg,rsp,req)!=RC_OK)
				CCPrintEx ( TRACE_LEVEL_API,"[DlglayEvtCB]process response is failed!\n");
		}
		break;
	case DLG_MSG_ACK:
		CCPrintEx ( TRACE_LEVEL_API,"[DlglayEvtCB]get ack\n");
		if(sipDlgMsgGetReq(&testmsg,&req)==RC_OK){
			CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]get a request!\n");
			if(callreqprocess(&testmsg,req)!=RC_OK)
				CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]process request is failed!\n");
		}
		break;
	case DLG_2XX_EXPIRE:
	case DLG_MSG_TRANSPORTERROR:
	case DLG_MSG_TX_TIMEOUT:
		if(handleTransErr(event,testmsg)!=RC_OK)
			CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]handle transport error is failed!\n");
		break;
	case DLG_MSG_NOMATCH_RSP:
		break;
	case DLG_MSG_NOMATCH_REQ:

#ifdef ICL_NO_MATCH_NOTIFY
		if(sipDlgMsgGetReq(&testmsg,&req)==RC_OK){
			/* �s�F�n�D */
			void *handle=NULL;
			pCCCall call=NULL;

			sipDlgMsgGetHandle(&testmsg,&handle);
			call=MatchCall((char *)sipTxGetCallid((TxStruct)handle));; /* for post only */
			
			if(NULL==call)
				ccCallNewEx(&call,sipTxGetCallid((TxStruct)handle));

			if(call!=NULL){
				/* set userdata for handle */
				UserData* txuser=(UserData*)calloc(1,sizeof(UserData));
				if( !txuser ){
					CCPrintEx ( TRACE_LEVEL_API, "[callreqprocess] Invalid argument : Memory allocation error\n" );
				}else{
					txuser->usertype=txu_call;
					txuser->data=(void*)call;
					sipTxSetUserData((TxStruct)handle,(void*)txuser);
				}
				dxHashAddEx(call->txdb,GetReqViaBranch(req),(void*)handle);
			/* post message */
			if(postprocess(call,req,NULL,SIP_NO_MATCH_REQ,NULL)!=RC_OK)
				CCERR("[cancelprocess] post msg is failed!\n");
				
			} 
			/* �s�F�n�D */
			/* ccCallFree(&call); */
		}
#endif /* ICL_NO_MATCH_NOTIFY */
		
		break;
	case DLG_MSG_TX_TERMINATED:
		{
			void *handle;
			UserData *txuser=NULL;
			TxStruct tx;
			pCCCall call=NULL;

			sipDlgMsgGetHandle(&testmsg,&handle);
			if(!handle){
				CCERR("[DlglayEvtCB]get a wrong dialog message\n");
				break;
			}
			tx=(TxStruct)handle;
			txuser=(UserData*)sipTxGetUserData(tx);
			if(!txuser){
				sipTxFree(tx);
				break;
			}
			if(txuser && txuser->usertype==txu_call){
				
				call=(pCCCall)txuser->data;
				if(call == NULL)
					call=MatchCall((char*)sipTxGetCallid(tx));

				if (call) {
					if (call->txdb) {
						req=sipTxGetOriginalReq(tx);
						dxHashDelEx(call->txdb,GetReqViaBranch(req),(void*)tx);
					}
				}

				FreeUserData(txuser);
			}
			sipTxFree(tx);
		break;
		}
	case DLG_CANCEL_EVENT:
		if(cancelprocess(testmsg)!=RC_OK)
			CCPrintEx( TRACE_LEVEL_API,"[DlglayEvtCB]process cancel is failed!\n");
		break;
	case DLG_MSG_TX_ETIME:
		{
			void *handle;
			UserData *txuser=NULL;
			TxStruct tx;
			pCCCall call=NULL;

			sipDlgMsgGetHandle(&testmsg,&handle);
			if(!handle){
				CCERR("[DlglayEvtCB]get a wrong dialog message\n");
				break;
			}
			tx=(TxStruct)handle;
			txuser=(UserData*)sipTxGetUserData(tx);

			if(txuser){
				if(txuser->usertype==txu_call){
				call=(pCCCall)txuser->data;
				}else{
					/* in dialog message */
					pSipDlg tmpdlg=(pSipDlg)txuser->data;
					void *rud=NULL;
					
					sipDlgGetUserData(&tmpdlg,&rud);

					call=(pCCCall)rud;
				}
				
				req=sipTxGetOriginalReq(tx);
				/* post to calllcontrol user */
				postprocess(call,req,NULL,SIP_TX_ETIME,NULL);

				/* update user profile to sipTx */
				UserProfileFree(tx->profile);
				tx->profile=UserProfileDup(call->outboundproxy);			
			}
		}
		break;
	default:
		break;
	};
}

/* input callid for new call object */
CCLAPI 
RCODE 
ccCallNew(IN OUT pCCCall *pcall)
{
	pCCCall call=NULL;
	char	buffer[256]={'\0'};

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallNew] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if( !g_cclocalIPAddress ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallNew] Run library init first!\n" );
		return RC_ERROR;
	}
	call=calloc(1,sizeof(CCCall));
	/* check memory alloc */
	if( !call ) {
		CCERR("[ccCallNew] Memory allocation error\n" );
		return RC_ERROR;
	}
	while(1){
		sprintf(buffer,"%s",(const char*)sipCallIDGen(NULL,g_cclocalIPAddress));
		if(MatchCall(buffer)==NULL) break;
	}
	call->callid=strDup(buffer);
	call->callstate=CALL_STATE_NULL;
	call->registcseq=1;
	call->dlglist=NULL;
	/*call->txlist=NULL;*/
	call->txdb=dxHashNew(128,0,DX_HASH_POINTER);
	call->laddr=strDup(g_cclocalIPAddress);
	call->usersetting=NULL;
	call->userdata=NULL;
	call->inviteDlg=NULL;
	
	/* add to dialog list */
	if(RC_OK!=dxHashAddEx(CallDBLst,call->callid,(void*)call)){
		ccCallFree(&call);
		CCERR("[ccCallNew] insert into calldb is failed!\n");
		return RC_ERROR;
	}
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccCallNew] total call:%d\n",dxHashSize(CallDBLst) );

	*pcall=call;
	return RC_OK;
}

CCLAPI 
RCODE 
ccCallFree(IN OUT pCCCall *pcall)
{
	pCCCall call=NULL;
	pSipDlg dlg=NULL;
	TxStruct tx;
	UserData *txuser;
	INT32 pos,dlgsize;
	
	Entry hashlist;
	char *key;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallFree] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallFree] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!call->callid) {
		CCWARN("[ccCallFree] Invalid argument : Null pointer to callid\n");
		return RC_ERROR;
	}

	/* remove from call db list */
	if (RC_OK!=dxHashDelEx(CallDBLst,call->callid,(void*)call)) 
		CCWARN("[ccCallFree] remove call from db is failed\n");
	
	CCPrintEx1( TRACE_LEVEL_API, "[ccCallFree] free %s\n",call->callid);

	if(call->callid)
		free(call->callid);

	if(call->laddr)
		free(call->laddr);

	if(call->dlglist){
		dlgsize=dxLstGetSize(call->dlglist);
		for(pos=0;pos<dlgsize;pos++){
			dlg=(pSipDlg)dxLstGetAt(call->dlglist,0);
			sipDlgFree(&dlg);
		}
		dxLstFree(call->dlglist,NULL);
	}
	/*
	if (call->txlist) {
		dlgsize=dxLstGetSize(call->txlist);
		for(pos=0;pos<dlgsize;pos++){
			tx=(TxStruct)dxLstGetAt(call->txlist,0);
			if(tx){
				txuser=(UserData*)sipTxGetUserData(tx);
				FreeUserData(txuser);
				sipTxFree(tx);
			}
		}
		dxLstFree(call->txlist,NULL);
	}*/
	if (call->txdb) {
		StartKeys(call->txdb);
		while ((key=NextKeys(call->txdb))) {
			hashlist=dxHashItemLst(call->txdb,key);
			while(hashlist){
				tx=(TxStruct)dxEntryGetValue(hashlist);
				if(tx){
					txuser=(UserData*)sipTxGetUserData(tx);
					sipTxSetUserData(tx,NULL);
					FreeUserData(txuser);			
					/*sipTxFree(tx);*/
				}
				hashlist=dxEntryGetNext(hashlist);
			}
		}
		dxHashFree(call->txdb,NULL);
	}

	if (call->usersetting) 
		ccUserFree(&call->usersetting);

	if(call->outboundproxy)
		UserProfileFree(call->outboundproxy);

	memset(call,0,sizeof(CCCall));
	free(call);
	*pcall=NULL;

	CCPrintEx1( TRACE_LEVEL_API, "[ccCallFree] total call:%d\n",dxHashSize(CallDBLst) );
	return RC_OK;

}

CCLAPI 
RCODE 
ccCallGetCallID(IN  pCCCall *const pcall ,OUT char *callid, IN OUT UINT32* length)
{

	pCCCall call=NULL;
	UINT32 len;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check input buffer size */
	if (!callid || !length) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : Null pointer to buffer\n" );
		return RC_ERROR;
	}

	if (!call->callid) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : Null pointer to call's callid\n" );
		return RC_ERROR;
	}
	len=strlen(call->callid);
	if(*length==0){
		*length=len+1;
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
	
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* clean buffer */
		memset(callid,0,*length);
		/* copy data */
		strncpy(callid,call->callid, len);
		*length=len+1;
		return RC_OK;
	}else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallID] Invalid argument : buffer is too small\n" );
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccCallGetCallState(IN  pCCCall *const pcall ,OUT ccCallState *cs)
{
	pCCCall call=NULL;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallState] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallState] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!cs) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallGetCallState] Invalid argument : Null pointer to callstate\n" );
		return RC_ERROR;
	}
	*cs=call->callstate;

	return RC_OK;
}

CCLAPI 
RCODE 
ccCallSetParent(IN pCCCall *const pcall,IN void * parent)
{
	pCCCall call=NULL;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!parent) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to parent\n" );
		return RC_ERROR;
	}
	call->userdata=parent;

	return RC_OK;	
}

/* get user data from call object */
CCLAPI 
RCODE 
ccCallGetParent(IN pCCCall *const pcall,OUT void **parent)
{
	pCCCall call=NULL;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!parent) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetParent] Invalid argument : Null pointer to parent\n" );
		return RC_ERROR;
	}
	*parent=call->userdata;

	return RC_OK;	
}

CCLAPI 
RCODE 
ccCallSetUserInfo(IN  pCCCall *const pcall,IN pCCUser user)
{
	pCCCall call=NULL;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetUserInfo] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetUserInfo] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}	
	if (call->usersetting) {
		if(RC_OK != ccUserFree(&call->usersetting)){
			CCWARN("[ccCallSetUserInfo] duplicate user is failed\n");
			return RC_ERROR;
		}
	}

	if (RC_OK!=ccUserDup(&user,&call->usersetting)){
		CCWARN("[ccCallSetUserInfo] duplicate user is failed\n");
		return RC_ERROR;
	}
	
	return RC_OK;
}

CCLAPI 
RCODE 
ccCallGetUserInfo(IN  pCCCall *const pcall,OUT pCCUser*user)
{
	pCCCall call=NULL;
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetUserInfo] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetUserInfo] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!user) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetUserInfo] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}
	
	*user=call->usersetting;
	return RC_OK;
}


/* set out bound sip server to call object */ 
RCODE 
ccCallSetOutBound(IN pCCCall *const pcall,IN const char *sipurl,IN const char* localip,IN UINT16 localport)
{
	UserProfile pfile=NULL;
	pCCCall call=NULL;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetOutBound] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetOutBound] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!sipurl) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallSetOutBound] Invalid argument : Null pointer to sipurl\n" );
		return RC_ERROR;
	}

	if(call->outboundproxy){
		UserProfileFree(call->outboundproxy);
		call->outboundproxy=NULL;
	}

	pfile=(UserProfile)calloc(1,sizeof(struct _UserProfile));
	if(NULL==pfile){
		CCERR("[ccCallSetOutBound]to alloc new memory for UserProfile is failed!");
		return RC_ERROR;
	}

	if(NULL==(pfile->proxyaddr=strDup(sipurl))){
		free(pfile);
		CCERR("[ccCallSetOutBound]to alloc new memory for UserProfile is failed!");
		return RC_ERROR;
	}

	pfile->useproxy=TRUE;
	pfile->localip=strDup(localip);
	pfile->localport=localport;
	call->outboundproxy=pfile;

	return RC_OK;
}

CCLAPI 
RCODE 
ccGetReferToUrl(IN pCCCall *const pcall,OUT char *rurl, IN OUT UINT32* length)
{
	char remotetarget[CCBUFLEN];
	UINT32 totallen;
	INT32 buflen=CCBUFLEN-1;

	pCCCall call;
	pSipDlg dlg;

	/* chech parameter */
	if( !pcall ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReferToUrl] NULL point to call!\n");
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReferToUrl] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	dlg=call->inviteDlg;
	if ( !dlg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReferToUrl] invite dialog is not found\n" );
		return RC_ERROR;
	}
	if(!rurl||!length){
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReferToUrl] buffer size is wrong!\n");
		return RC_ERROR;
	}

	if (sipDlgGetRemoteTarget(&dlg,remotetarget,&buflen)!=RC_OK) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReferToUrl] from-tag is not found!\n");
		return RC_ERROR;
	}
	totallen=strlen(remotetarget);
	if(*length==0){
		*length=totallen+1;
		return RC_ERROR;
	}
	
	if(totallen < *length ){
		sprintf(rurl,"%s",remotetarget);
		*length=totallen+1;
		return RC_OK;
	}else{
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReferToUrl] buffer size is too small!\n");
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccGetReplaceID(IN pCCCall *const pcall,OUT char *rid, IN OUT UINT32* length,IN BOOL uas)
{
	char *callid,totag[CCBUFLEN],fromtag[CCBUFLEN];
	UINT32 totallen;
	INT32 buflen=CCBUFLEN-1;

	pCCCall call;
	pSipDlg dlg;

	/* chech parameter */
	if( !pcall ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] NULL point to call!\n");
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReplaceID] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	dlg=call->inviteDlg;
	if ( !dlg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReplaceID] invite dialog is not found\n" );
		return RC_ERROR;
	}

	if(!rid||!length){
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] buffer size is wrong!\n");
		return RC_ERROR;
	}

	callid=call->callid;
	if (!callid) {
		CCERR ("call-id is null!\n");
		return RC_ERROR;

	}
	if (sipDlgGetLocalTag(&dlg,fromtag,&buflen)!=RC_OK) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] from-tag is not found!\n");
		return RC_ERROR;
	}

	buflen=255;
	if (sipDlgGetRemoteTag(&dlg,totag,&buflen)!=RC_OK) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] to-tag is not found!\n");
		return RC_ERROR;
	}

	totallen=18+strlen(callid)+strlen(totag)+strlen(fromtag);
	if(*length==0){
		*length=totallen+1;
		return RC_ERROR;
	}
	
	if(totallen < *length ){
		if (uas==TRUE) 
			sprintf(rid,"%s;to-tag=%s;from-tag=%s",callid,fromtag,totag);
		else
			sprintf(rid,"%s;to-tag=%s;from-tag=%s",callid,totag,fromtag);
		*length=totallen+1;
		return RC_OK;
	}else{
		*length=totallen+1;
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] buffer size is too small!\n");
		return RC_ERROR;
	}
}

CCLAPI 
RCODE 
ccGetInviteDlg(IN pCCCall *const pcall,OUT pSipDlg *pdlg)
{
	pCCCall call;
	/* chech parameter */
	if( !pcall ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccGetReplaceID] NULL point to call!\n");
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReplaceID] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!pdlg) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetReplaceID] Invalid argument : Null pointer to pdlg\n" );
		return RC_ERROR;
	}
	*pdlg=call->inviteDlg;
	return RC_OK;

}

CCLAPI
RCODE 
ccCallCheckComplete(IN pCCCall *const pcall,OUT BOOL *progress)
{
	pCCCall call;
	pSipDlg dlg;
	TxStruct tx;
	INT32 num,pos;
	BOOL	dlgproc;
	TXSTATE txstate;

	Entry hashlist;
	char *key;

	/* check parameter */
	if( !pcall ) {
		CCPrintEx( TRACE_LEVEL_API,"[ccCallCheckComplete] NULL point to call!\n");
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallCheckComplete] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!progress) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallCheckComplete] Invalid argument : Null pointer to progress\n" );
		return RC_ERROR;
	}
	/* check dialog first */
	*progress=TRUE;
	num=dxLstGetSize(call->dlglist);
	for(pos=0;pos<num;pos++){
		dlg=(pSipDlg)dxLstPeek(call->dlglist,pos);
		if(NULL==dlg)
			continue;
		if (RC_OK==sipDlgCheckComplete(&dlg,&dlgproc)) {
			if (FALSE==dlgproc) {
				*progress=FALSE;
				return RC_OK;
			}
		}
	}

	/* check tx 
	num=dxLstGetSize(call->txlist);
	for(pos=0;pos<num;pos++){
		tx=(TxStruct)dxLstPeek(call->txlist,pos);
		txstate=sipTxGetState(tx);
		if ((txstate==TX_TRYING)||(txstate==TX_PROCEEDING)) {
			*progress=FALSE;
			return RC_OK;
		}
	}*/
	if (call->txdb) {
		StartKeys(call->txdb);
		while ((key=NextKeys(call->txdb))) {
			hashlist=dxHashItemLst(call->txdb,key);
			while(hashlist){
				tx=(TxStruct)dxEntryGetValue(hashlist);
				hashlist=dxEntryGetNext(hashlist);
				txstate=sipTxGetState(tx);
				if(tx){
					if ((txstate==TX_TRYING)||(txstate==TX_PROCEEDING)) {
						*progress=FALSE;
						return RC_OK;
					}
				}
			}
		}
	}

	/* if no match, return RC_ERROR */
	return RC_OK;
}

/* basic call api*/
CCLAPI 
RCODE 
ccMakeCall(IN  pCCCall *const pcall,IN const char *dest,IN pCont msgbody,IN SipReq refer)
{
	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	char buffer[CCBUFLEN]={'\0'};
	char *contactaddr=NULL,*displayname=NULL;
	pCCUser userinfo=NULL;
	UINT32 len=255;
	SipFrom *from=NULL;
	SipTo *to=NULL;

	SipReq postreq = NULL;
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMakeCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMakeCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( IsEmpty(dest) ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMakeCall] Invalid argument : Null pointer to destination URL\n" );
		return RC_ERROR;
	}
	/* check call state */
	/*
	if(call->callstate != CALL_STATE_NULL){
		CCWARN("[ccMakeCall] error call state !\n" );
		return RC_ERROR;
	}
	*/
	/* only null or disconnect is allowed to make a new call */
	if(!((call->callstate == CALL_STATE_NULL)||(call->callstate == CALL_STATE_DISCONNECT))){
		CCWARN("[ccMakeCall] error call state !\n" );
		return RC_ERROR;
	}
	
	/* get user setting */
	getUserUrl(call,buffer,&len);
	
	if ( RC_OK != sipDlgNew( &_pdlg, call->callid,buffer,dest, NULL, GetReqFromTag(refer))) {
		CCPrintEx( TRACE_LEVEL_API,"[ccMakeCall] new dialog object failed!\n");
		return RC_ERROR;
	}
	
	/* update CSeq due to the same call-id */
	if(NULL!=call->inviteDlg){
		UINT32 oldcseq=1;
		sipDlgGetLocalCseq(&call->inviteDlg,&oldcseq);
		/* oldcseq++; */
		sipDlgSetLocalCseq(&_pdlg,oldcseq);
	}

	/* new invite request */
	if ( RC_OK != sipDlgNewReq( &_pdlg,&_preq,INVITE)) {
		sipDlgFree(&_pdlg);
		CCPrintEx( TRACE_LEVEL_API,"[ccMakeCall] new \"INVITE\" request failed!\n");
		return RC_ERROR;
	}

	/* modify request */
	ccReqModify(&_preq,refer);

	if(sipReqGetHdr(refer,Record_Route) != NULL)
		sipDlgSetRRTrue(&_pdlg);

	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buffer,&len)==RC_OK)
			displayname=strDup(buffer);
		len=CCBUFLEN-1;
		if(ccUserGetContact(&userinfo,buffer,&len)==RC_OK)
			contactaddr=strDup(buffer);
	
		if(NULL!=displayname){
			/* add displayname */
			from=(SipFrom*)sipReqGetHdr(_preq,From);
			if(from && from->address){
				if(NULL==from->address->display_name)
					from->address->display_name=strDup(displayname);
			}
			free(displayname);
			displayname=NULL;
		}
	}

	/*add TO remote displayname */
	if (NULL != g_remoteDisplayname){
		to=(SipTo*)sipReqGetHdr(_preq,To);
		if(to && to->address){
			if(NULL==to->address->display_name)
				to->address->display_name=strDup(g_remoteDisplayname);
		}
	}

	/* add contact */
	if(contactaddr){
		sprintf(buffer,"Contact:%s\r\n",contactaddr);
		free(contactaddr);
	}else
		sprintf(buffer,"Contact:<sip:%s:%d>\r\n",call->laddr,g_cclistenport);
	
	if ( RC_OK != sipReqAddHdrFromTxt(_preq,Contact,buffer) ){
		sipReqFree(_preq);
		return RC_ERROR;
	}

	/* add body and content-related headers */
	ReqAddBody(_preq,msgbody);

	postreq = sipReqDup(_preq);
	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		sipDlgFree(&_pdlg);
		CCPrintEx(TRACE_LEVEL_API,"[ccMakeCall] send \"INVITE\" request failed!\n");
		return RC_ERROR;
	}

	/* add conn to call */
	/* add new connection to call */
	if(RC_OK!=AddDLGtoCALL(call,_pdlg))
		CCWARN("[ccMakeCall] add new dialog to call is failed\n");

	call->inviteDlg=_pdlg;
	/* set connection link to call */
	sipDlgSetUserData(&_pdlg,(void*)call);

	/* modify call state */
	call->callstate=CALL_STATE_INITIAL;

	/* post message */
	if(postprocess(call,postreq,NULL,CALL_STATE,NULL)!=RC_OK)
		CCERR("[ccMakeCall] post msg is failed!\n");

	if(postreq)
		sipReqFree(postreq);

	return RC_OK;
}

CCLAPI 
RCODE 
ccAnswerCall(IN pCCCall *const pcall,IN pCont msgbody)
{
	return ccAnswerCallEx(pcall,msgbody,NULL);
}

CCLAPI 
RCODE 
ccAnswerCallEx(IN pCCCall *const pcall,IN pCont msgbody,IN SipRsp rsp)
{
	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	SipRsp _prsp = NULL;
	SipRsp postrsp = NULL;
	char buffer[CCBUFLEN]={'\0'};
	char *contactaddr=NULL;
	BOOL postflag=FALSE;
	UINT32 len=CCBUFLEN-11;


	char *displayname=NULL;
	pCCUser userinfo=NULL;
	SipTo *to=NULL;
	

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccAnswerCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccAnswerCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if(call->callstate != CALL_STATE_OFFER){
		CCWARN("[ccAnswerCall] error call state !\n" );
		return RC_ERROR;
	}

	/* get dialog */
	_pdlg=call->inviteDlg;	
	if( !_pdlg ) {
		CCERR("[ccAnswerCall] Invite dialog is not found!\n" );
		return RC_ERROR;
	}

	_preq=GetReqFromCall(_pdlg);

	if( !_preq){
		CCWARN("[ccAnswerCall] request is not found\n" );
		return RC_ERROR;
	}

	/* new invite response */
	if(RC_OK != sipDlgNewRsp(&_prsp,_preq,"200","ok")){
		CCWARN("[ccAnswerCall] to generate response is failed\n" );
		return RC_ERROR;
	}

	/* modify response */
	/* add contact */
	if(call->usersetting != NULL)
		if(RC_OK==ccUserGetContact(&call->usersetting,buffer,&len))
			contactaddr=strDup(buffer);
	if(contactaddr){
		sprintf(buffer,"Contact:%s\r\n",contactaddr);
		free(contactaddr);
	}else
		sprintf(buffer,"Contact:<sip:%s:%d>\r\n",call->laddr,g_cclistenport);
	sipRspAddHdrFromTxt(_prsp,Contact,buffer);

	/* set to displayname */
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buffer,&len)==RC_OK)
			displayname=strDup(buffer);

		/* add displayname */
		if(NULL!=displayname){
			to=(SipTo*)sipRspGetHdr(_prsp,To);
			if(to && to->address){
				if(NULL==to->address->display_name)
					to->address->display_name=strDup(displayname);
			}
			free(displayname);
			displayname=NULL;
		}
	}

	/* modify response */
	ccRspModify(_prsp,rsp);

	/* body related headers */
	RspAddBody(_prsp,msgbody);

	postrsp = sipRspDup(_prsp);
	/* send response */
	if ( RC_OK != sipDlgSendRsp( &_pdlg, _prsp)) {
		sipRspFree(_prsp);
		CCPrintEx(TRACE_LEVEL_API,"[ccAnswerCall] send 200 OK failed!\n");
		return RC_ERROR;
	}

	/* update call state */
	if(call->callstate==CALL_STATE_OFFER){
		call->callstate=CALL_STATE_ANSWER;
		postflag=TRUE;
	}

	if(postflag==TRUE){
		/* post message */
		if(postprocess(call,NULL,postrsp,CALL_STATE,NULL)!=RC_OK)
			CCERR("[ccAnswerCall] post msg is failed!\n");
	}
	if(postrsp != NULL)
		sipRspFree(postrsp);
	
	return RC_OK;
}

CCLAPI 
RCODE 
ccAckCall(IN  pCCCall *const pcall,IN pCont msgbody)
{

	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	SipReq postreq  = NULL;

	char buffer[CCBUFLEN]={'\0'};
	char *displayname=NULL;
	pCCUser userinfo=NULL;
	UINT32 len=255;
	SipFrom *from=NULL;

	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccAckCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccAckCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	
	/* check call state */
	if((call->callstate != CALL_STATE_ANSWERED) && ((call->callstate != CALL_STATE_CONNECT))) {
		CCWARN("[ccAckCall] call state is not answered\n" );
		return RC_ERROR;
	}
	
	/* get dialog */
	
	_pdlg=call->inviteDlg;

	if( !_pdlg){
		CCWARN("[ccAckCall] no such dialog for ack\n" );
		return RC_ERROR;
	}
	/* new ACK request */
	if ( RC_OK != sipDlgNewReq( &_pdlg,&_preq,ACK)) {
		CCPrintEx( TRACE_LEVEL_API,"[ccAckCall] new \"ACK\" request failed!\n");
		return RC_ERROR;
	}
	/* modify request */
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buffer,&len)==RC_OK)
			displayname=strDup(buffer);

		/* add displayname */
		if(NULL!=displayname){	
			from=(SipFrom*)sipReqGetHdr(_preq,From);
			if(from && from->address){
				if(NULL==from->address->display_name)
					from->address->display_name=strDup(displayname);
			}
			free(displayname);
			displayname=NULL;
		}
	}
	
	/* add body */
	ReqAddBody(_preq,msgbody);

	postreq = sipReqDup(_preq);
	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		CCPrintEx( TRACE_LEVEL_API,"[ccAckCall] send \"ACK\" request failed!\n");
		return RC_ERROR;
	}

	/* update call state */
	if(call->callstate==CALL_STATE_ANSWERED){
		call->callstate=CALL_STATE_CONNECT;

		/* post message */
		if(postprocess(call,postreq,NULL,CALL_STATE,NULL)!=RC_OK)
			CCERR("[ccAckCall] post msg is failed!\n");
	}

	if(postreq)
		sipReqFree(postreq);
	
	return RC_OK;
}

/* send bye or cancel depend on call state */
CCLAPI 
RCODE 
ccDropCall(IN  pCCCall *const pcall,IN pCont msgbody)
{

	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;

	char buffer[CCBUFLEN]={'\0'};
	char *displayname=NULL;
	pCCUser userinfo=NULL;
	UINT32 len=255;
	SipFrom *from=NULL;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccDropCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccDropCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	
	/* check call state */
	if((call->callstate < CALL_STATE_PROCEED)||(call->callstate > CALL_STATE_ANSWER)||(call->callstate==CALL_STATE_OFFER)){
		CCWARN("[ccDropCall] Error call state !\n" );
		return RC_ERROR;
	}

	/* get dialog */
	_pdlg=call->inviteDlg;
	
	if( !_pdlg ) {
		CCERR("[ccDropCall] Invalid argument : dialog is not found\n" );
		return RC_ERROR;
	}
	/* new BYE request */
	if((call->callstate==CALL_STATE_CONNECT)
		||(call->callstate==CALL_STATE_ANSWERED)
		||(call->callstate==CALL_STATE_ANSWER)){
		if ( RC_OK != sipDlgNewReq( &_pdlg,&_preq,BYE)) {
			CCPrintEx( TRACE_LEVEL_API,"[ccDropCall] new \"BYE\" request failed!\n");
			return RC_ERROR;
		}
	}else if((call->callstate==CALL_STATE_PROCEED)||(call->callstate==CALL_STATE_RINGING)){
		if ( RC_OK != sipDlgNewReq( &_pdlg,&_preq,CANCEL)) {
			CCPrintEx( TRACE_LEVEL_API,"[ccDropCall] new \"CANCEL\" request failed!\n");
			return RC_ERROR;
		}
	}else{
		CCPrintEx( TRACE_LEVEL_API,"[ccDropCall] callstate is not match!\n");
		return RC_ERROR;
	}

	/* modify request */
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buffer,&len)==RC_OK)
			displayname=strDup(buffer);
		
		/* add displayname */
		if(NULL!=displayname){	
			from=(SipFrom*)sipReqGetHdr(_preq,From);
			if(from && from->address){
				if(NULL==from->address->display_name)
					from->address->display_name=strDup(displayname);
			}
			free(displayname);
			displayname=NULL;
		}
	}
	
	/* body related headers */
	ReqAddBody(_preq,msgbody);
	
	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		CCPrintEx( TRACE_LEVEL_API,"[ccDropCall] send \"BYE\" request failed!\n");
		return RC_ERROR;
	}
	return RC_OK;
}

/* send 1xx message ex.180 */
CCLAPI 
RCODE 
ccRingCall(IN  pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody)
{
	return ccRingCallEx(pcall,code,reason,msgbody,NULL);
}

CCLAPI 
RCODE ccRingCallEx(IN pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN SipRsp rsp)
{
	pCCCall call;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	SipRsp _prsp = NULL;
	char buffer[CCBUFLEN]={'\0'};
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRingCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRingCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if( call->callstate != CALL_STATE_OFFER) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRingCall] Invalid call state!\n" );
		return RC_ERROR;
	}
	
	/* get dialog */

	_pdlg=call->inviteDlg;	
	if( !_pdlg ) {
		CCERR("[ccRingCall] Invite dialog is not found!\n" );
		return RC_ERROR;
	}

	_preq=GetReqFromCall(_pdlg);

	if( !_preq){
		CCWARN("[ccRingCall] request is not found\n" );
		return RC_ERROR;
	}

	if (GetReqMethod(_preq)!=INVITE) {
		CCWARN("[ccRingCall] it is not a invite request !\n");
		return RC_ERROR;
	}

	if(( code<101 )||(code>199)) {
		CCERR("[ccRingCall] status code must in 101~199!\n" );
		return RC_ERROR;
	}

	sprintf(buffer,"%d",code);
	if(RC_OK != sipDlgNewRsp(&_prsp,_preq,buffer,(char*)reason)){
		CCWARN("[ccRingCall] to generate response is failed\n" );
		return RC_ERROR;
	}

	/* add contact */
	if(call->usersetting != NULL){
		UINT32 len=256;
		char *contactaddr=NULL;
		if(RC_OK==ccUserGetContact(&call->usersetting,buffer,&len))
			contactaddr=strDup(buffer);
		if(contactaddr){
			sprintf(buffer,"Contact:%s\r\n",contactaddr);
			free(contactaddr);
		}else
			sprintf(buffer,"Contact:<sip:%s:%d>\r\n",call->laddr,g_cclistenport);
		sipRspAddHdrFromTxt(_prsp,Contact,buffer);
	}
	
	/* modify response */
	ccRspModify(_prsp,rsp);

	/* body related headers */
	RspAddBody(_prsp,msgbody);
	/* send response */
	if ( RC_OK != sipDlgSendRsp( &_pdlg, _prsp)) {
		sipRspFree(_prsp);
		CCPrintEx(TRACE_LEVEL_API,"[ccRingCall] send 1XX rsp failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}
/* send 302 response */
CCLAPI 
RCODE 
ccForwardCall(IN  pCCCall *const pcall,IN const char *newtarget,IN pCont msgbody)
{

	pCCCall call;
	pSipDlg _pdlg = NULL;
	SipReq _preq =NULL;
	SipRsp _prsp = NULL;
	char buffer[CCBUFLEN];
	
	SipRsp postrsp =NULL;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccForwardCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccForwardCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if( call->callstate != CALL_STATE_OFFER) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccForwardCall] Invalid call state!\n" );
		return RC_ERROR;
	}

	/* get dialog */
	_pdlg=call->inviteDlg;	
	if( !_pdlg ) {
		CCERR("[ccForwardCall] Invite dialog is not found!\n" );
		return RC_ERROR;
	}

	_preq=GetReqFromCall(_pdlg);

	if( !_preq){
		CCWARN("[ccForwardCall] request is not found\n" );
		return RC_ERROR;
	}

	if (GetReqMethod(_preq)!=INVITE) {
		CCWARN("[ccForwardCall] it is not a invite request !\n");
		return RC_ERROR;
	}

	if( IsEmpty(newtarget) ) {
		CCERR("[ccForwardCall] must input a new target!\n" );
		return RC_ERROR;
	}

	if(RC_OK != sipDlgNewRsp(&_prsp,_preq,"302","Move temporarily")){
		CCWARN("[ccForwardCall] to generate response is failed\n" );
		return RC_ERROR;
	}

	postrsp = sipRspDup( _prsp);

	/* modify response */
	/* add contact */
	sprintf(buffer,"Contact:%s\r\n",newtarget);
	if(sipRspAddHdrFromTxt(_prsp,Contact,buffer) != RC_OK){
		sipRspFree(_prsp);
	}
	/* body related headers */
	RspAddBody(_prsp,msgbody);

	/* send response */
	if ( RC_OK != sipDlgSendRsp( &_pdlg, _prsp)) {
		sipRspFree(_prsp);
		CCPrintEx(TRACE_LEVEL_API,"[ccForwardCall] send 302 Move temporarily failed!\n");
		return RC_ERROR;
	}

	/* update call state */
	call->callstate=CALL_STATE_DISCONNECT;	

	/* post message */
	if(postprocess(call,NULL,postrsp,CALL_STATE,NULL)!=RC_OK)
		CCERR("[ccForwardCall] post msg is failed!\n");
	
	if (postrsp)
		sipRspFree(postrsp);

	return RC_OK;
}

/* send 4XX~6XX response */
CCLAPI 
RCODE 
ccRejectCall(IN  pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody)
{
	return ccRejectCallEx(pcall,code,reason,msgbody,NULL);
}

CCLAPI 
RCODE 
ccRejectCallEx(IN  pCCCall *const pcall,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN SipRsp rsp)
{
	pCCCall call;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	SipRsp _prsp = NULL;
	char buffer[CCBUFLEN];
	BOOL bePost=FALSE;

	SipRsp postrsp=NULL;
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRejectCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRejectCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if(( call->callstate != CALL_STATE_OFFER)&&(call->callstate != CALL_STATE_CONNECT)) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRejectCall] Invalid call state!\n" );
		return RC_ERROR;
	}

	/* get dialog */
	_pdlg=call->inviteDlg;	
	if( !_pdlg ) {
		CCERR("[ccRejectCall] Invite dialog is not found!\n" );
		return RC_ERROR;
	}

	_preq=GetReqFromCall(_pdlg);
	if( !_preq){
		CCWARN("[ccRejectCall] request is not found\n" );
		return RC_ERROR;
	}

	if (GetReqMethod(_preq)!=INVITE) {
		CCWARN("[ccRejectCall] it is not a invite request !\n");
		return RC_ERROR;
	}

	if((code<400)||(code>699)||(code==481)) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRejectCall] status code must 400~600!\n" );
		return RC_ERROR;
	}	

	if( !reason ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccRejectCall] no reason for response!\n" );
		return RC_ERROR;
	}

	sprintf(buffer,"%d",code);
	if(RC_OK != sipDlgNewRsp(&_prsp,_preq,buffer,(char*)reason)){
		CCWARN("[ccRejectCall] to generate response is failed\n" );
		return RC_ERROR;
	}

	/* modify response */
	ccRspModify(_prsp,rsp);
	
	/* body related headers */
	RspAddBody(_prsp,msgbody);

	postrsp = sipRspDup(_prsp);
	/* send response */
	if ( RC_OK != sipDlgSendRsp( &_pdlg, _prsp)) {
		sipRspFree(_prsp);
		CCPrintEx(TRACE_LEVEL_API,"[ccRejectCall] send 4xx rsp failed!\n");
		return RC_ERROR;
	}

	/* update call state */
	if(call->callstate==CALL_STATE_OFFER){
		call->callstate=CALL_STATE_DISCONNECT;

		if((code!=401)||(code!=407))
			bePost=TRUE;
	}else if(call->callstate==CALL_STATE_CONNECT){
		if(code==408){
			call->callstate=CALL_STATE_DISCONNECT;
			bePost=TRUE;
		}
	}

	/* post message */
	if(bePost==TRUE)
		if(postprocess(call,NULL,postrsp,CALL_STATE,NULL)!=RC_OK)
			CCERR("[ccRejectCall] post msg is failed!\n");
	
	if (postrsp)
		sipRspFree(postrsp);
	
	return RC_OK;	
}

CCLAPI 
RCODE 
ccModifyCall(IN  pCCCall *const pcall,IN pCont msgbody)
{
	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccModitfyCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccModitfyCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if(call->callstate != CALL_STATE_CONNECT){
		CCWARN("[ccModitfyCall] error call state !\n" );
		return RC_ERROR;
	}
	/* get invite dialog to send invite */
	_pdlg=call->inviteDlg;
	if (!_pdlg) {
		CCERR("[ccModitfyCall] Invalid argument : Null pointer to original dialog!\n");
		return RC_ERROR;
	}

	/* new refer request */
	if ( RC_OK != ccNewReqFromCallLeg(&call,&_preq,INVITE,NULL,msgbody,&_pdlg)) {
		CCPrintEx( TRACE_LEVEL_API,"[ccModitfyCall] new \"INVITE\" request failed!\n");
		return RC_ERROR;
	}

	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		CCPrintEx(TRACE_LEVEL_API,"[ccModitfyCall] send \"INVITE\" request failed!\n");
		return RC_ERROR;
	}

	return RC_OK;

}

CCLAPI 
RCODE 
ccUpdateCall(IN  pCCCall *const pcall,IN pCont msgbody)
{
	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUpdateCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccUpdateCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	/* check call state */
	if(call->callstate != CALL_STATE_CONNECT){
		CCWARN("[ccUpdateCall] error call state !\n" );
		return RC_ERROR;
	}
	/* get invite dialog to send invite */
	_pdlg=call->inviteDlg;
	if (!_pdlg) {
		CCERR("[ccUpdateCall] Invalid argument : Null pointer to original dialog!\n");
		return RC_ERROR;
	}

	/* new refer request */
	if ( RC_OK != ccNewReqFromCallLeg(&call,&_preq,SIP_UPDATE,NULL,msgbody,&_pdlg)) {
		CCPrintEx( TRACE_LEVEL_API,"[ccUpdateCall] new \"UPDATE\" request failed!\n");
		return RC_ERROR;
	}

	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		CCPrintEx(TRACE_LEVEL_API,"[ccUpdateCall] send \"UPDATE\" request failed!\n");
		return RC_ERROR;
	}

	return RC_OK;

}

CCLAPI 
RCODE 
ccTransferCall(IN pCCCall *const pcall,IN const char* target,IN const char* replaceid,IN pCont msgbody)
{
	pCCCall call=NULL;
	pSipDlg _pdlg = NULL;
	SipReq _preq = NULL;
	char buffer[2*CCBUFLEN]={'\0'};
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccTransferCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccTransferCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if ( IsEmpty(target) ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccTransferCall] Invalid argument : Null pointer to target\n" );
		return RC_ERROR;
	}

	/* check call state */
	if(call->callstate != CALL_STATE_CONNECT){
		CCWARN("[ccTransferCall] error call state !\n" );
		return RC_ERROR;
	}
	/* get invite dialog to send invite */
	_pdlg=call->inviteDlg;
	if (!_pdlg) {
		CCERR("[ccTransferCall] Invalid argument : Null pointer to original dialog!\n");
		return RC_ERROR;
	}

	/* new refer request */
	if ( RC_OK != ccNewReqFromCallLeg(&call,&_preq,REFER,NULL,msgbody,&_pdlg)) {
		CCPrintEx( TRACE_LEVEL_API,"[ccTransferCall] new \"refer\" request failed!\n");
		return RC_ERROR;
	}
	/* modify request */
	/* add refer and replace if it presents */
	if (replaceid) {
		char *capreplaceid=EscapeURI(replaceid);
		if (capreplaceid) {
			sprintf(buffer,"<%s?Replaces=%s>",target,capreplaceid);
			free(capreplaceid);
		}
	}else
		sprintf(buffer,"<%s>",target);
	if(RC_OK!=ccReqAddHeader(_preq,Refer_To,buffer)){
		sipReqFree(_preq);
		CCPrintEx( TRACE_LEVEL_API,"[ccTransferCall] adding refer-to is failed!\n");
		return RC_ERROR;
	}

	/* add referred-by */
	sprintf(buffer,"<%s>",GetReqFromUrl(_preq));
	if(RC_OK!=ccReqAddHeader(_preq,Referred_By,buffer))
		CCWARN("[ccTransferCall] adding referred-by is failed!\n");
	
	/* send request */
	if ( RC_OK != sipDlgSendReqEx( &_pdlg, _preq ,call->outboundproxy)) {
		sipReqFree(_preq);
		CCPrintEx(TRACE_LEVEL_API,"[ccTransferCall] send \"REFER\" request failed!\n");
		return RC_ERROR;
	}

	return RC_OK;
}

/*Initiate callcontrol Layer library */
CCLAPI 
RCODE 
CallCtrlInit(IN CallEvtCB eventCB,IN SipConfigData config,IN const char *realm)
{
	
	if (CallDBLst) {
		CCPrintEx ( TRACE_LEVEL_API, "[CallCtrlInit] callctrl layer initiated!\n" );
		return RC_OK;
	}
	g_callevtcb=eventCB;
	/*CallDBLst=dxLstNew(DX_LST_POINTER);*/
	CallDBLst=dxHashNew(CCDBSIZE,0,DX_HASH_POINTER);

	if( !CallDBLst ){
		CCPrintEx ( TRACE_LEVEL_API, "[CallCtrlInit] callcontrol layer failed!\n" );
		return RC_ERROR;
	}
	if(g_ccrealm){
		free(g_ccrealm);
		g_ccrealm=NULL;
	}
	if(g_cclocalIPAddress){
		free(g_cclocalIPAddress);
		g_cclocalIPAddress=NULL;
	}
	if (RC_OK==sipDlgInit(DlglayEvtCB,config)) {
		g_cclocalIPAddress=strDup(config.laddr);
		g_cclistenport=config.tcp_port;

		if(realm)
			g_ccrealm=strDup(realm);
		else
			CCWARN("[CallCtrlInit]set realm is failed!\n");
		CCPrintEx1 ( TRACE_LEVEL_API, "[CallCtrlInit] callctrl layer initiated!(version:%s)\n",CCVERSION );
		return RC_OK;
	}else{
		dxHashFree(CallDBLst,NULL);
		CallDBLst=NULL;
		return RC_ERROR;
	}
}

CCLAPI	
RCODE 
CallCtrlClean(void)
{
	pCCCall call;

	Entry hashlist;
	char *key;

	g_callevtcb = NULL;

	printf("[CallCtrl] [CallCtrlClean]\n");
	/* clean DlgDBLst */
	if(CallDBLst){
		StartKeys(CallDBLst);
		while ((key=NextKeys(CallDBLst))) {
			hashlist=dxHashItemLst(CallDBLst,key);
			while(hashlist){
				call=(pCCCall)dxEntryGetValue(hashlist);
				hashlist=dxEntryGetNext(hashlist);
				ccCallFree(&call);
			}
		}
		
		TCRPrint ( TRACE_LEVEL_API, "[CallCtrl] [CallCtrlClean] total call:%d\n",dxHashSize(CallDBLst) );
		printf("[CallCtrl] [CallCtrlClean] total call:%d\n",dxHashSize(CallDBLst));
		dxHashFree(CallDBLst,NULL);
		CallDBLst=NULL;
	}

	if(g_cclocalIPAddress){
		free(g_cclocalIPAddress);
		g_cclocalIPAddress=NULL;
	}

	if(g_ccrealm){
		free(g_ccrealm);
		g_ccrealm=NULL;
	}

	CCPrintEx ( TRACE_LEVEL_API, "[CallCtrlClean] callctrl layer cleaned!\n" );
	printf("[CallCtrlClean] callctrl layer cleaned!\n");
	return sipDlgClean(); 
}

/* dispatch callcontrol events */
CCLAPI 
RCODE 
CallCtrlDispatch(IN const int timeout)
{
	return sipDlgDispatch(timeout);
}

/* new request */
CCLAPI 
RCODE 
ccNewReqFromCall(IN  pCCCall *const pcall,OUT SipReq *preq,IN SipMethodType method,IN const char *remoteurl,IN pCont msgbody)
{
	pCCCall call=NULL;
	SipReq	req = NULL;
	SipReqLine *pReqline=NULL;
	pCCUser userinfo=NULL;

	char buf[CCBUFLEN]={'\0'};
	char *username=NULL,*displayname=NULL,*servip=NULL,*laddr=NULL;
	char *tourl=NULL;
	INT32 pos;
	UINT32 len=CCBUFLEN-1;
	

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if ( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( !call->callid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] Invalid argument : Null pointer to callid\n" );
		return RC_ERROR;
	}
	
	if ( !preq ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] Invalid argument : Null pointer to request pointer\n" );
		return RC_ERROR;
	}

	if ( IsEmpty(remoteurl) ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] Invalid argument : Null pointer to remote url\n" );
		return RC_ERROR;
	}

	/* check method type */
	if ((method==OPTIONS)
		||(method==REGISTER)
#ifdef ICL_NO_MATCH_NOTIFY
		||(method==SIP_NOTIFY)
#endif
		||(method==SIP_INFO)
		||(method==SIP_PUBLISH)
		||(method==SIP_MESSAGE)) 
		;
	else{
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCall] method is not allowed\n" );
		return RC_ERROR;
	}

	req=sipReqNew();
	if( !req ){
		CCWARN("[ccNewReqFromCall] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* request line */
	pReqline=(SipReqLine*)calloc(1,sizeof(SipReqLine));
	if( !pReqline ){
		CCWARN("[ccNewReqFromCall] Memory allocation error\n" );
		sipReqFree(req);
		return RC_ERROR;
	}
	pReqline->iMethod=method;
	pReqline->ptrRequestURI=strDup(remoteurl);
	pReqline->ptrSipVersion=strDup(DLG_SIP_VERSION);
	
	if(sipReqSetReqLine(req,pReqline) != RC_OK){
		CCERR("[ccNewReqFromCall] To set request line is failed!\n");
		sipReqFree(req);
		sipReqLineFree(pReqline);
		return RC_ERROR;
	}
	sipReqLineFree(pReqline);
	
	/* add call id */
	sprintf(buf,"Call-ID:%s",call->callid);
	if(sipReqAddHdrFromTxt(req,Call_ID,buf)!=RC_OK)
		CCERR("[ccNewReqFromCall] To add siphdr callid is failed\n" );
	
	/* add cseq */
	sprintf(buf,"CSeq:%ud %s",call->registcseq,sipMethodTypeToName(method));
	if(sipReqAddHdrFromTxt(req,CSeq,buf)!=RC_OK)
		CCERR("[ccNewReqFromCall] To add siphdr cseq is failed\n" );
	
	if((REGISTER==method)||(SIP_PUBLISH==method)){
		call->registcseq++;
		/* add expire header */
		/*sprintf(buf,"Expires:%d",CCREGISTER_EXPIRE);
		if(sipReqAddHdrFromTxt(req,Expires,buf)!=RC_OK)
			CCERR("[ccNewReqFromCall] To add siphdr expires is failed\n" );
		 */
	}
	
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetUserName(&userinfo,buf,&len)==RC_OK)
			username=strDup(buf);
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buf,&len)==RC_OK)
			displayname=strDup(buf);
		len=CCBUFLEN-1;
		if(ccUserGetAddr(&userinfo,buf,&len)==RC_OK)
			laddr=strDup(buf);
	}

	/* add from */
	pos=sprintf(buf,"From:");
	
	if(displayname != NULL){
		pos+=sprintf(buf+pos,"\"%s\"",displayname);
		free(displayname);
	}
	
	if(username != NULL){
		pos+=sprintf(buf+pos,"<sip:%s@",username);
	}else
		pos+=sprintf(buf+pos,"<sip:");

	if(laddr != NULL){
		char *newtag=taggen();
		sprintf(buf+pos,"%s>;tag=%s",laddr,newtag);
		free(laddr);
		laddr=NULL;
		if(newtag) free(newtag);
	}else{
		char *newtag=taggen();
		sprintf(buf+pos,"%s>;tag=%s",call->laddr,newtag);
		if(newtag) free(newtag);
	}

	if(sipReqAddHdrFromTxt(req,From,buf)!=RC_OK)
		CCERR("[ccNewReqFromCall] To add siphdr from is failed\n" );
	
	/* add max-forward */
	sprintf(buf,"Max-Forwards:%d",SIP_MAX_FORWARDS);
	if(sipReqAddHdrFromTxt(req,Max_Forwards,buf)!=RC_OK)
		CCERR("[ccNewReqFromCall] To add siphdr Max_Forwards is failed\n" );

	/* add to */
	tourl=NewNOPortURL(remoteurl);
	if(NULL==tourl){
		CCERR("[ccNewReqFromCall] To set to url is failed!\n");
		sipReqFree(req);
		return RC_ERROR;
	}
	
	if (username&&(REGISTER==method)){ 
		servip=strchr(tourl,'@');
		if (servip) 
			servip++;
		else{
			servip=strchr(tourl,':');
			if (servip) servip++;
		}
		if (servip) 
			sprintf(buf,"To:<sip:%s@%s>",username,servip);
		else
			sprintf(buf,"To:%s",tourl);
	}else
		sprintf(buf,"To:%s",tourl);

	free(tourl);
	tourl=NULL;
	
	if (username){ 
		free(username);
		username=NULL;
	}
	
	if(sipReqAddHdrFromTxt(req,To,buf)!=RC_OK)
		CCERR("[ccNewReqFromCall] To add siphdr to is failed\n" );

	/* add user agent */
	sprintf(buf,"User-Agent:%s\r\n\0",SIP_USER_AGENT);
	if(sipReqAddHdrFromTxt(req,User_Agent,buf)!=RC_OK)
		CCWARN("[ccNewReqFromCall] To add siphdr callid failed\n" );

	/* add body */
	if(msgbody){
		if(RC_OK!=ReqAddBody(req,msgbody))
			CCWARN("[ccNewReqFromCall] To add message body is failed\n" );
	}
	*preq=req;
	return RC_OK;
}

/* new response */
CCLAPI 
RCODE 
ccNewRspFromCall(IN  pCCCall *const pcall,OUT SipRsp *prsp,IN const unsigned short code,IN const char *reason,IN pCont msgbody,IN const char* reqid,IN pSipDlg dlg)
{
	pCCCall call=NULL;
	SipRsp rsp=NULL;
	char	*callid=NULL;
	SipReq req=NULL;
	SipMethodType method;
	SipRspStatusLine *pStatusline=NULL;	
	SipCSeq *pCseq=NULL;
	SipFrom *pFrom=NULL;
	SipTo	*pTo=NULL;
	SipRecordRoute *pRecroute=NULL;
	char buf[CCBUFLEN],*contactaddr=NULL;
	INT32 size,pos;
	UINT32 len=CCBUFLEN-1;

	/*Entry txlist;*/
	TxStruct tx;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if ( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( !call->callid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : Null pointer to callid\n" );
		return RC_ERROR;
	}

	if((code>999)||(code<101)){
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : wrong statuscode \n" );
		return RC_ERROR;
	}

	if ( ! reason ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : Null pointer to callid\n" );
		return RC_ERROR;
	}	

	if ( ! reqid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}
	/* get request from tx */
	if (dlg) {
		if (RC_OK!=sipDlgGetReqByBranchID(&dlg,&req,reqid)) {
			CCWARN ( "[ccNewRspFromCall] request is not found\n" );
			/*return RC_ERROR;*/
		}
	}else{
	
		tx=(TxStruct)dxHashItem(call->txdb,reqid);
		req=sipTxGetOriginalReq(tx);
	}
	/* not found in specified tx or dialog */
	if (!req) {
		size=dxLstGetSize(call->dlglist);
		for(pos=0;pos<size;pos++){
			dlg=(pSipDlg)dxLstPeek(call->dlglist,pos);
			if (RC_OK!=sipDlgGetReqByBranchID(&dlg,&req,reqid)) break;
			req=NULL;
			dlg=NULL;
		}
	}
	/* not found in call ,return error */
	if (!req) {
		CCERR ("[ccNewRspFromCall] request is not found\n" );
		return RC_ERROR;
	}

	/* check callid */
	callid=(char*)sipReqGetHdr(req,Call_ID);
	if(strCmp(call->callid,callid)!=0){
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] callid is not match\n" );
		return RC_ERROR;
	}

	/* check to header */
	pTo=(SipTo*)sipReqGetHdr(req,To);
	if( !pTo ){
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] to is NULL\n" );
		return RC_ERROR;
	}
	/* check cseq */
	pCseq=(SipCSeq*)sipReqGetHdr(req,CSeq);
	if(! pCseq){
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewRspFromCall] CSeq is NULL\n" );
		return RC_ERROR;
	}

	/* contruct response */
	rsp=sipRspNew();
	if( !rsp ){
		CCWARN("[ccNewRspFromCall] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* set status-line */
	pStatusline=calloc(1,sizeof(SipRspStatusLine));
	if( !pStatusline ){
		CCWARN("[ccNewRspFromCall] Memory allocation error\n" );
		sipRspFree(rsp);
		return RC_ERROR;
	}
	sprintf(buf,"%d",code);
	pStatusline->ptrReason=strDup(reason);
	pStatusline->ptrStatusCode=strDup(buf);
	pStatusline->ptrVersion=strDup(DLG_SIP_VERSION);
	if( sipRspSetStatusLine(rsp,pStatusline) != RC_OK ){
		CCERR("[ccNewRspFromCall] To set request line is failed!\n");
		sipRspFree(rsp);
		sipRspStatusLineFree(pStatusline);
		return RC_ERROR;
	}
	sipRspStatusLineFree(pStatusline);
	
	/* add callid*/
	if( sipRspAddHdr(rsp,Call_ID,(void*)callid) != RC_OK)
		CCWARN("[ccNewRspFromCall] To add siphdr callid failed\n");
	/* add cseq */
	if( sipRspAddHdr(rsp,CSeq,(void*)pCseq) != RC_OK)
		CCWARN("[ccNewRspFromCall] To add siphdr cseq failed\n");

	method=GetReqMethod(req);
	/* add contact if method is invite ,subscribe/refer,notify */
	if ((code>199)&&(code<300)) {
		if ((method==INVITE)||(method==SIP_SUBSCRIBE)||(method==REFER)||(method==SIP_NOTIFY)) {
			len=CCBUFLEN-11;
			if (call->usersetting) {
				if(RC_OK==ccUserGetContact(&call->usersetting,buf,&len))
					contactaddr=strDup(buf);
			}
			if(contactaddr){
				sprintf(buf,"Contact:%s\r\n",contactaddr);
				free(contactaddr);
			}else
				sprintf(buf,"Contact:<sip:%s:%d>\r\n",call->laddr,g_cclistenport);
			sipRspAddHdrFromTxt(rsp,Contact,buf);
		}
	}

	/* add from */
	pFrom=(SipFrom*)sipReqGetHdr(req,From);
	if( sipRspAddHdr(rsp,From,(void*)pFrom) != RC_OK)
		CCWARN("[ccNewRspFromCall] To add siphdr from failed\n");

	/* add to */
	if( sipRspAddHdr(rsp,To,(void*)pTo) !=RC_OK)
		CCWARN("[ccNewRspFromCall] To add siphdr to failed\n");

	/* match dialog when invite & subscribe if no totag */
	if (!GetRspToTag(rsp)) {
		if (dlg) {
			
			char localtag[CCBUFLEN]={'\0'};
			INT32 len=CCBUFLEN-1;
			
			if(RC_OK==sipDlgGetLocalTag(&dlg,localtag,&len)){
					AddRspToTag(rsp,(const char*)localtag);
			}else{
				char *newtag=taggen();
				AddRspToTag(rsp,(const char*)newtag);
				if(newtag) free(newtag);
			}
			
		}
	}

	/*copy Via*/
	sipRspAddHdr(rsp,Via,sipReqGetHdr(req,Via));
	
	/* rec-route,except out of dialog message and register */
	if ( dlg && (method!=REGISTER)) {
		pRecroute=sipReqGetHdr(req,Record_Route);
		if(pRecroute)
			if(sipRspAddHdr(rsp,Record_Route,(void*)pRecroute) != RC_OK)
				CCWARN("[ccNewRspFromCall] To add siphdr from failed\n");
	}
	
	/* add user agent */
	sprintf(buf,"User-Agent:%s\r\n\0",SIP_USER_AGENT);
	if(sipRspAddHdrFromTxt(rsp,User_Agent,buf)!=RC_OK)
		CCWARN("[ccNewRspFromCall] To add siphdr callid failed\n" );
	
	/* body related headers */
	RspAddBody(rsp,msgbody);

	/* set response */
	*prsp=rsp;

	return RC_OK;
}

/* send request */
CCLAPI 
RCODE 
ccSendReq(IN  pCCCall *const pcall,IN SipReq req,IN pSipDlg dlg)
{
	pCCCall call;
	TxStruct tx;
	UserData *ud;
	RCODE rCode;
	SipReq _preq;
	SipMethodType method;

	/* check parameter */
	if ( ! pcall ) {
		
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	
	if( !req ){
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Invalid argument : Null pointer to Sip Request\n" );
		return RC_ERROR;
	}

	/*if (GetReqMethod(req)==INVITE) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Request Method must not be invite\n" );
		return RC_ERROR;
	}*/

	/* check method */
	method=GetReqMethod(req);
	/*
	if (dlg&&(method!=SIP_SUBSCRIBE)&&(method!=REFER)) {
		DlgState dlgstate;
		if (RC_OK==sipDlgGetDlgState(&dlg,&dlgstate)) {
			if ((dlgstate!=DLG_STATE_CONFIRM)&&(method!=PRACK)&&(method!=SIP_UPDATE)) {
				dlg=NULL;
			}
		}
	}
	*/

	if(dlg){
		DlgState dlgstate;
		if (RC_OK==sipDlgGetDlgState(&dlg,&dlgstate)) {
			if ((dlgstate!=DLG_STATE_EARLY)&&(method==PRACK)) {
				CCWARN("PRACK should be use in early dialog!");
			}
		}
	}

	/* check must in dialog message */
	switch(method) {
	case INVITE:
	case ACK:
	case BYE:
	case REFER:
	case SIP_SUBSCRIBE:
#ifndef ICL_NO_MATCH_NOTIFY
	case SIP_NOTIFY:
#endif
	case SIP_UPDATE:
	case PRACK: /* early dialog */
		if (!dlg) {
			CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Sip Request must be sent in dialog\n" );
			return RC_ERROR;
		}
		break;
	default:
		break;
	}

	if ( !dlg ) {
		/* new tx to send request */
		ud=calloc(1,sizeof(UserData));
		if( !ud ){
			CCPrintEx ( TRACE_LEVEL_API, "[ccSendReq] Memory allocation error\n" );
			return RC_ERROR;
		}
		ud->usertype=txu_call;
		ud->data=(void*)call;
		/* new Tx to send request */
		_preq=sipReqDup(req);
		tx=sipTxClientNew(_preq,NULL,call->outboundproxy,(void*)ud);
	
		if(!tx){
			sipReqFree(_preq);
			CCERR("[ccSendReq] create Tx is failed!\n");
			return RC_ERROR;
		}
		/* add tx to list  */
		rCode=dxHashAddEx(call->txdb,sipTxGetTxid(tx),(void*)tx);
	}else{
		/* send request */
		_preq=sipReqDup(req);
		if ( RC_OK != sipDlgSendReqEx( &dlg, _preq ,call->outboundproxy)) {
			sipReqFree(_preq);
			CCPrintEx(TRACE_LEVEL_API,"[ccSendReq] send request failed!\n");
			return RC_ERROR;
		}
		rCode=RC_OK;
	}

	
#ifdef _DEBUG
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccSendReq] send a request\n%s\n",sipReqPrint(req) );
#else
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccSendReq] send a (%s)\n",PrintRequestline(req) );
#endif
	return rCode;
}

/* new response */
CCLAPI 
RCODE 
ccSendRsp(IN  pCCCall *const pcall,IN SipRsp rsp)
{
	pCCCall call;
	char *branch;
	TxStruct tx=NULL;
	pSipDlg dlg;
	SipMethodType method;
	SipRsp _prsp=NULL;
	
	/*Entry txlist;*/

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendRsp] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendRsp] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( !rsp ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendRsp] Invalid argument : Null pointer to Sip response\n" );
		return RC_ERROR;
	}

	branch=GetRspViaBranch(rsp);
	if( !branch ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSendRsp] Invalid argument : Null pointer to Via Branch\n" );
		return RC_ERROR;
	}

	method=GetRspMethod(rsp);
	if ((method==OPTIONS)
		||(method==REGISTER)
		||(method==SIP_INFO)
		||(method==SIP_PUBLISH)
		||(method==SIP_MESSAGE)) {
		if(GetRspToTag(rsp)==NULL)
			dlg=NULL;
		else
			dlg=matchDialogByRsp(call,rsp,1);
	}else
		dlg=matchDialogByRsp(call,rsp,1);
	
	if ( !dlg ) {
		
		tx=(TxStruct)dxHashItem(call->txdb,branch);
		
		if( !tx ) {
			CCWARN("[ccSendRsp] transaction is not found\n" );
			return RC_ERROR;
		}

		if( sipTxGetState(tx)>=TX_COMPLETED) {
			CCWARN("[ccSendRsp] transaction is terminated\n" );
			return RC_ERROR;
		}
		
		/*
		 * RFC 3261,8.2.6.2 p50
		 * uas must add a tag to the To header.
		 */

#ifdef ICL_NO_MATCH_NOTIFY
		if (NULL==GetRspToTag(rsp)&&(method!=SIP_NOTIFY)){
#else
		if (NULL==GetRspToTag(rsp)) {
#endif
			char *newtag=taggen();
			if(newtag){
				AddRspToTag(rsp,newtag);
				free(newtag);
			}
		}
		_prsp=sipRspDup(rsp);
		sipTxSendRsp(tx,_prsp);
		
	}else{
		/* send response */
		_prsp=sipRspDup(rsp);
		if ( RC_OK != sipDlgSendRsp( &dlg, _prsp)) {
			sipRspFree(_prsp);
			CCPrintEx(TRACE_LEVEL_API,"[ccSendRsp] send a response is failed!\n");
			return RC_ERROR;
		}
	}
	
	
#ifdef _DEBUG
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccSendRsp] send a response\n%s\n",sipRspPrint(rsp) );
#else
	CCPrintEx2 (TRACE_LEVEL_API, "[ccSendRsp] send a (%s) to %s\n",PrintStatusline(rsp) ,GetRspViaSentBy(rsp));
#endif
	return RC_OK;
}

CCLAPI 
RCODE 
ccNewCallLegFromCall(IN pCCCall *const pcall,IN const char* remoteurl, OUT pSipDlg *Legid)
{
	pCCCall call=NULL;
	pSipDlg dlg=NULL;
	char buf[2*CCBUFLEN]={'\0'};
	UINT32 len;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewCallLegFromCall] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if ( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewCallLegFromCall] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( !call->callid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewCallLegFromCall] Invalid argument : Null pointer to callid\n" );
		return RC_ERROR;
	}

	if (!remoteurl) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewCallLegFromCall] Invalid argument : Null pointer to remoteurl\n" );
		return RC_ERROR;
	}

	if (!Legid) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewCallLegFromCall] Invalid argument : Null pointer to Legid\n" );
		return RC_ERROR;
	}

	/* get user url */
	len=CCBUFLEN;
	getUserUrl(call,buf,&len);
	
	if ( RC_OK != sipDlgNew( &dlg, call->callid,buf,remoteurl, NULL, NULL)) {
		CCPrintEx( TRACE_LEVEL_API,"[ccNewReqFromCallLeg] new dialog object failed!\n");
		return RC_ERROR;
	}
	/* add new dialog to call */
	if(RC_OK!=AddDLGtoCALL(call,dlg))
		CCERR("[ccNewReqFromCallLeg] add new dialog to call is failed\n");
	/* set connection link to call */
	if (RC_OK!=sipDlgSetUserData(&dlg,(void*)call))
		CCERR("[ccNewReqFromCallLeg] setting new dialog is failed\n");
	/* set request and call leg */
	*Legid=dlg;
	return RC_OK;

}

CCLAPI 
RCODE ccFreeCallLeg(IN pSipDlg *Legid)
{
	pCCCall call=NULL;
	pSipDlg callleg;
	void *ud;

	/* check parameter */
	if ( ! Legid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccFreeCallLeg] Invalid argument : Null pointer to pCallleg\n" );
		return RC_ERROR;
	}
	callleg=*Legid;
	if ( !callleg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccFreeCallLeg] Invalid argument : Null pointer to callleg\n" );
		return RC_ERROR;
	}	

	if (RC_OK!=sipDlgGetUserData(&callleg,&ud))
		CCERR("[ccNewReqFromCallLeg] getting new dialog is failed\n");

	if(ud){
		INT32 pos,size;
		call=(pCCCall)ud;
		size=dxLstGetSize(call->dlglist);
		for (pos=0;pos<size;pos++)
		{
			if(callleg==(pSipDlg)dxLstPeek(call->dlglist,pos)){
				dxLstGetAt(call->dlglist,pos);
				break;
			}/* if callleg */
		} /* for pos */
	}/* if ud */

	return sipDlgFree(Legid);
}

CCLAPI 
RCODE 
ccNewReqFromCallLeg(IN  pCCCall *const pcall,
					OUT SipReq *preq,
					IN SipMethodType method,
					IN const char* remoteurl,
					IN pCont msgbody,
					IN OUT pSipDlg *Legid)
{
	pCCCall call=NULL;
	pCCUser userinfo=NULL;

	pSipDlg dlg=NULL;

	SipReq	req = NULL;
	SipFrom *from;

	char buf[2*CCBUFLEN]={'\0'};
	char *displayname=NULL,*contacturl=NULL;
	UINT32 len=CCBUFLEN-1;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if ( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}

	if( !call->callid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to callid\n" );
		return RC_ERROR;
	}
	
	if ( !preq ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to request pointer\n" );
		return RC_ERROR;
	}

	if (!Legid ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to dlg pointer\n" );
		return RC_ERROR;
	}

	dlg=*Legid;
	if (!dlg&&!remoteurl) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccNewReqFromCallLeg] Invalid argument : Null pointer to remote url\n" );
		return RC_ERROR;
	}

	userinfo=call->usersetting;
	if(!dlg){
		/* check method type */
		if ((method==REFER)
			||(method==SIP_SUBSCRIBE)) 
			;
		else{
			CCWARN ("[ccNewReqFromCallLeg] method is not allowed\n" );
			return RC_ERROR;
		}
		/* no old dialog is used ,so new dialog for it */
		len=CCBUFLEN;
		getUserUrl(call,buf,&len);
		
		if ( RC_OK != sipDlgNew( &dlg, call->callid,buf,remoteurl, NULL, NULL)) {
			CCPrintEx( TRACE_LEVEL_API,"[ccNewReqFromCallLeg] new dialog object failed!\n");
			return RC_ERROR;
		}
	}else{
		/* check method and dialog state */
		/*
		DlgState	state;
		if(sipDlgGetDlgState(&dlg,&state)==RC_OK){
			if(state!=DLG_STATE_CONFIRM){
				CCWARN("[ccNewReqFromCallLeg] Error dialog state !\n" );
				return RC_ERROR;
			}
		}
		*/
		if ((method==REGISTER)||(method==CANCEL)) {
			CCWARN("[ccNewReqFromCallLeg] Error call state !\n" );
			return RC_ERROR;
		}
	}

	/* new invite request */
	if ( RC_OK != sipDlgNewReq( &dlg,&req,method)) {
		
		/* Free dlg is created by ccNewReqFromCallLeg() */
		if(*Legid!=dlg){
		sipDlgFree(&dlg);
		}

		CCPrintEx( TRACE_LEVEL_API,"[ccNewReqFromCallLeg] new request failed!\n");
		return RC_ERROR;
	}
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetDisplayName(&userinfo,buf,&len)==RC_OK)
			displayname=strDup(buf);
		len=CCBUFLEN-1;
		if(ccUserGetContact(&userinfo,buf,&len)==RC_OK)
			contacturl=strDup(buf);

		if(NULL!=displayname){	
			from=(SipFrom*)sipReqGetHdr(req,From);
			if(from && from->address){
				if(NULL==from->address->display_name)
					from->address->display_name=strDup(displayname);
			}
			free(displayname);
			displayname=NULL;
		}
	}

	/* add contact */
	switch(method) {
	case INVITE:
	case REFER:
	case SIP_SUBSCRIBE:
	case SIP_NOTIFY:
	case SIP_UPDATE:
	case SIP_INFO:
		if(contacturl){
			sprintf(buf,"Contact:%s\r\n",contacturl);
			free(contacturl);
		}else
			sprintf(buf,"Contact:<sip:%s:%d>\r\n",call->laddr,g_cclistenport);
		sipReqAddHdrFromTxt(req,Contact,buf);
		break;
	default:
		if (contacturl) 
			free(contacturl);
	} 
	
	/* add req body */
	ReqAddBody(req,msgbody);

	/* add new dialog to call */
	if(RC_OK!=AddDLGtoCALL(call,dlg))
		CCWARN("[ccNewReqFromCallLeg] add new dialog to call is failed\n");

	/* set connection link to call */
	sipDlgSetUserData(&dlg,(void*)call);

	/* set request and callleg */
	*preq=req;
	*Legid=dlg;

	return RC_OK;
}

RCODE 
CallCtrlPostMsg(IN CallEvtType event,IN pCCMsg pMsg)
{

	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[CallCtrlPostMsg] Invalid argument : Null pointer to message" );
		return RC_ERROR;
	}
	
	if (g_callevtcb != NULL)
		g_callevtcb(event,(void*)pMsg);

	return RC_OK;
}

CCLAPI
RCODE 
ccCallNewEx(IN OUT pCCCall *pcall,IN const char* callid)
{
	pCCCall call=NULL;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallNewEx] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if( !g_cclocalIPAddress ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccCallNewEx] Run library init first!\n" );
		return RC_ERROR;
	}
	call=calloc(1,sizeof(CCCall));
	/* check memory alloc */
	if( !call ) {
		CCERR("[ccCallNewEx] Memory allocation error\n" );
		return RC_ERROR;
	}

	call->callid=strDup(callid);
	call->callstate=CALL_STATE_NULL;
	call->registcseq=1;
	call->dlglist=NULL;
	/*call->txlist=NULL;*/
	call->txdb=dxHashNew(1024,0,DX_HASH_POINTER);
	call->laddr=strDup(g_cclocalIPAddress);
	call->usersetting=NULL;
	call->userdata=NULL;
	call->inviteDlg=NULL;
	
	/* add to dialog list */
	if(RC_OK!=dxHashAddEx(CallDBLst,call->callid,(void*)call)){
		ccCallFree(&call);
		CCERR("[ccCallNewEx] insert into calldb is failed!\n");
		return RC_ERROR;
	}
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccCallNewEx] total call:%d\n",dxHashSize(CallDBLst) );

	*pcall=call;
	return RC_OK;
}

RCODE 
ccCallSetCallID(IN  pCCCall *const pCall,IN char *callid)
{
	pCCCall call=NULL;

	if( !pCall)
		return RC_ERROR;
	
	call=*pCall;

	if( !call )
		return RC_ERROR;

	if(call->callid){
		/* remove from hash table */
		if (RC_OK!=dxHashDelEx(CallDBLst,call->callid,(void*)call)) 
			CCWARN("[ccCallSetCallID] remove call from db is failed\n");
		free(call->callid);
	}
	call->callid=strDup(callid);
	
	/* add to hash table */
	if(RC_OK!=dxHashAddEx(CallDBLst,call->callid,(void*)call)){
		CCERR("[ccCallSetCallID] insert into calldb is failed!\n");
		return RC_ERROR;
	}

	return RC_OK;

}

pSipDlg matchDialogByRsp(IN pCCCall call,IN SipRsp rsp,IN INT32 mtype)
{
	char *remotetag,*localtag;
	
	INT32 pos,dlgsize;
	pSipDlg _pdlg=NULL;
	
	if (!call || !rsp ) {
		return NULL;
	}

	dlgsize=dxLstGetSize(call->dlglist);
	if(mtype==0){
		remotetag=GetRspToTag(rsp);
		localtag=GetRspFromTag(rsp);
	}else{
		localtag=GetRspToTag(rsp);
		remotetag=GetRspFromTag(rsp);
	}
	

	for(pos=0;pos<dlgsize;pos++){
		_pdlg=(pSipDlg)dxLstPeek(call->dlglist,pos);

		if(NULL==_pdlg) continue;
		
		if(sipDlgBeMatchTag(&_pdlg,localtag,remotetag))
			break;
		else
			_pdlg = NULL;
	}
	return _pdlg;
}

SipReq GetReqFromCall(IN pSipDlg dlg)
{

	TxStruct tx;

	if (!dlg) 
		return NULL;

	if (RC_OK==sipDlgGetInviteTx(&dlg,TRUE,&tx))
		return sipTxGetOriginalReq(tx);

	return NULL;
}

RCODE getUserUrl(IN pCCCall call,OUT char* buffer,IN OUT UINT32* length)
{
	char *username=NULL,*laddr=NULL,*scheme=NULL;
	pCCUser userinfo=NULL;
	char buf[256];
	UINT32 len=255;

	if(NULL == call)
		return RC_ERROR;

	if(NULL == buffer)
		return RC_ERROR;

	/* get user setting */
	userinfo=call->usersetting;
	if(userinfo){
		len=CCBUFLEN-1;
		if(ccUserGetUserName(&userinfo,buf,&len)==RC_OK)
			username=strDup(buf);
		
		len=CCBUFLEN-1;
		if(ccUserGetAddr(&userinfo,buf,&len)==RC_OK)
			laddr=strDup(buf);

		len=CCBUFLEN-1;
		if(ccUserGetScheme(&userinfo,buf,&len)==RC_OK)
			scheme=strDup(buf);
	}else{
		scheme=strDup("sip");
	}
	
	/* new dialog to send invite */
	if(username&&(strlen(username)>0)){
		if(NULL !=laddr){
			/*sprintf(buf,"%s:%s@%s",scheme,username,laddr);*/
			if(strICmp(scheme,"sip")==0)
                                sprintf(buf,"%s:%s@%s",scheme,username,laddr);
                        else
                                sprintf(buf,"%s:%s",scheme,username);

		}else{
			if(strICmp(scheme,"sip")==0)
				sprintf(buf,"%s:%s@%s",scheme,username,call->laddr);
			else
				sprintf(buf,"%s:%s",scheme,username);
		}
	}else
		sprintf(buf,"%s:%s",scheme,call->laddr);

	/* free username and laddr */
	if(NULL != username)
		free(username);
	
	if(NULL != laddr)
		free(laddr);

	if(NULL != scheme){
		free(scheme);
	}

	len=strlen(buf);
	/* clean buffer */
	memset(buffer,0,*length);
	/* if buffer size is enough ,copy remote target to buffer */
	if(len < *length ){
		/* copy data */
		strcpy(buffer,buf);
		*length=len+1;
		return RC_OK;
	}else{
		return RC_ERROR;
	}
}

const char* callstateToStr(IN ccCallState cs)
{
	switch (cs)
	{
	case CALL_STATE_INITIAL:
		return "CALL_STATE_INITIAL";

	case CALL_STATE_PROCEED:
		return "CALL_STATE_PROCEED";

	case CALL_STATE_RINGING:
		return  "CALL_STATE_RINGING";

	case CALL_STATE_ANSWERED:
		return "CALL_STATE_ANSWERED";

	case CALL_STATE_CONNECT:
		return "CALL_STATE_CONNECT";

	case CALL_STATE_OFFER:
		return "CALL_STATE_OFFER";

	case CALL_STATE_ANSWER:
		return "CALL_STATE_ANSWER";

	case CALL_STATE_DISCONNECT:
		return "CALL_STATE_DISCONNECT";

	case CALL_STATE_NULL:
	default:
		return "CALL_STATE_NULL";
	}
}

RCODE ccGetCallDBSize(OUT INT32* ret)
{
	
	if (CallDBLst) {
		*ret=dxHashSize(CallDBLst);
	}else
		*ret=0;
	return RC_OK;
}

RCODE 
ccSetCallToDB(IN OUT pCCCall *pcall)
{
	pCCCall call=NULL;
	pSipDlg dlg=NULL;
	int pos,dlgsize;
	
	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSetCallToDB] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	
	call=*pcall;
	if( !call ) {
		CCERR("[ccSetCallToDB] Memory allocation error\n" );
		return RC_ERROR;
	}

	if( !g_cclocalIPAddress ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccSetCallToDB] Run library init first!\n" );
		return RC_ERROR;
	}
	
	/* add to call list */
	if(RC_OK!=dxHashAddEx(CallDBLst,call->callid,(void*)call)){
		CCERR("[ccSetCallToDB] insert into calldb is failed!\n");
		return RC_ERROR;
	}

	/* add all dialog to dlg-database */
	if(call->dlglist){
		dlgsize=dxLstGetSize(call->dlglist);
		for(pos=0;pos<dlgsize;pos++){
			dlg=(pSipDlg)dxLstPeek(call->dlglist,pos);
			/* call API to remove dlg from db */
			if(dlg!=NULL) sipDlgSetDB(&dlg);
		}
	}
	CCPrintEx1 ( TRACE_LEVEL_API, "[ccSetCallToDB] total call:%d\n",dxHashSize(CallDBLst) );

	return RC_OK;
}

RCODE 
ccGetCallFromDB(IN OUT pCCCall *pcall)
{
	pCCCall call=NULL;
	pSipDlg dlg=NULL;
	TxStruct tx;
	UserData *txuser;
	INT32 pos,dlgsize;
	
	Entry hashlist;
	char *key;

	/* check parameter */
	if ( ! pcall ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetCallFromDB] Invalid argument : Null pointer to pCall\n" );
		return RC_ERROR;
	}
	call=*pcall;
	if( !call ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccGetCallFromDB] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	if (!call->callid) {
		CCWARN("[ccGetCallFromDB] Invalid argument : Null pointer to callid\n");
		return RC_ERROR;
	}

	/* remove from call db list */
	if (RC_OK!=dxHashDelEx(CallDBLst,call->callid,(void*)call)) 
		CCWARN("[ccGetCallFromDB] remove call from db is failed\n");

	/* remove all dialog from dlg-database */
	if(call->dlglist){
		dlgsize=dxLstGetSize(call->dlglist);
		for(pos=0;pos<dlgsize;pos++){
			dlg=(pSipDlg)dxLstPeek(call->dlglist,pos);
			/* call API to remove dlg from db */
			if(dlg!=NULL) sipDlgRemoveFromDB(&dlg);
		}
	}

	/* remove all from txdb */
	if (call->txdb) {
		StartKeys(call->txdb);
		while ((key=NextKeys(call->txdb))) {
			hashlist=dxHashItemLst(call->txdb,key);
			while(hashlist){
				tx=(TxStruct)dxEntryGetValue(hashlist);
				hashlist=dxEntryGetNext(hashlist);
				if(tx){
					txuser=(UserData*)sipTxGetUserData(tx);
					FreeUserData(txuser);
					sipTxSetUserData(tx,NULL);
					/*sipTxFree(tx);*/
				}
			}
		}
	}

	return RC_OK;
}

RCODE ccSetREALM(IN const char *newrealm)
{
	if(NULL!=newrealm){
		if(NULL!=g_ccrealm)
			free(g_ccrealm);
		g_ccrealm=strDup(newrealm);
		if(NULL==g_ccrealm)
			return RC_ERROR;
		else
			return RC_OK;
	}else
		return RC_ERROR;
}

RCODE ccSetRemoteDisplayName(IN const char *displayname)
{
	if(NULL!=displayname){
		if(NULL!=g_remoteDisplayname)
			free(g_remoteDisplayname);
		g_remoteDisplayname=strDup(displayname);
		if(NULL==g_remoteDisplayname)
			return RC_ERROR;
		else
			return RC_OK;
	}else
		return RC_ERROR;
}

BOOL CheckContParam(IN SipReq req)
{
	BOOL handling=TRUE;
	SipContentDisposition *cdisp;
	SipContentLanguage *clang;
	SipContEncoding *cenc;

	if(NULL==req) return TRUE;
	/* content-disposition */
	cdisp=(SipContentDisposition*)sipReqGetHdr(req,Content_Disposition);
	if(cdisp && cdisp->paramList){
		SipParam* tmpparam;
		
		tmpparam=cdisp->paramList;
		while(tmpparam && tmpparam->name && tmpparam->value){
			if(strICmp(tmpparam->name,"handling")==0){
				if(strICmp(tmpparam->value,"optional")==0){
					handling=FALSE;
				}
			}

			tmpparam=tmpparam->next;
		}
			
		
	}else
		return TRUE;
	
	
	if(handling){
		if(cdisp && cdisp->disptype ){/* exist */
			if(strICmp(cdisp->disptype,"session")==0) ;
			else if (strICmp(cdisp->disptype,"render")==0) ;
			else if(strICmp(cdisp->disptype,"alert")==0) ;
			else
				return FALSE;
		}
		
		/* content-language */
		clang=(SipContentLanguage*)sipReqGetHdr(req,Content_Language);
		if(clang && clang->langList && clang->langList->str){
			if(NULL==strstr(ACCEPT_LANGUAGE,clang->langList->str))
				return FALSE;
		}
		/* content-encoding */
		cenc=(SipContEncoding*)sipReqGetHdr(req,Content_Encoding);
		if(cenc && cenc->sipCodingList && cenc->sipCodingList->coding){/* exist */
			if(NULL==strstr(ACCEPT_ENCODING,cenc->sipCodingList->coding))
				return FALSE;
		}
	}/* end of handling */

	return TRUE;
}


