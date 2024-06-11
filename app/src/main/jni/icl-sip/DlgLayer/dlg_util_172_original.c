/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * dlg_util.c
 *
 * $Id: dlg_util.c,v 1.25 2009/03/11 07:46:14 tyhuang Exp $
 */
#include <stdio.h>
#include <assert.h>

#include <sipTx/sipTx.h>
#include <sip/sip.h>
#include <adt/dx_lst.h>
#include <adt/dx_hash.h>

#include "dlg_util.h" 
#include "dlg_cm.h"
#include "dlg_msg.h"
#include "dlg_sipmsg.h"

struct sipDlgObj{
	char		*callid;

	char		*localurl;
	char		*localtag;
	UINT32		localcseq;

	char		*remotedisplayname; /* for TTCN */
	char		*remoteurl;
	char		*remotetag;
	char		*remotetarget;
	UINT32		remotecseq;
	
	DxLst		routeset;
	BOOL		secure;

	DlgState	state;
	DxLst		txlist;
	DxLst		eventlist;

	/* dialog user data */
	void		*userdata;

	/* remove top most route*/
	BOOL		bRR;

	/* authorization data */
	SipReq		authreq;

	/* support PRACK */
	UINT32		invitecseq;
	UINT32		rseq;
	DxLst		racklist;

	/* support dialog event package */
	UINT32		dialogid;
	BOOL		beInitiator;

	/* support replace */
	SipMethodType createmethod;
};

DxHash	DLGDBLst=NULL;
DlgEvtCB g_dlgevtcb=NULL;
char *g_allowevent=NULL;

RCODE sipDlgPostMsg(IN DlgEvtType event,IN pDlgMsg pMsg);

RCODE sipDlgAddTx(pSipDlg *const pDlg,TxStruct tx);
RCODE sipDlgDelTx(pSipDlg *const pDlg,TxStruct tx);
RCODE sipDlgAddEvent(pSipDlg *const pDlg,const char*);
RCODE sipDlgRemoveEvent(pSipDlg *const pDlg,const char*);

void dlgFreeUserData(UserData *ud);

pSipDlg GetMatchDlg(SipMethodType method,const char *callid,const char* fromtag,const char *totag);
pSipDlg GetReplacedDlg(SipReplaces *replaces);

pSipDlg sipDlgMatchByTx(TxStruct txData);

void FreeMemory(void *pv){
	assert( pv != NULL );
#ifdef _DEBUG
	memset(pv,0,sizeof(pv));
#endif
	free(pv);
}

BOOL BeDestroyDlg(int statuscode);
BOOL BeCreateDlg(SipMethodType method);
RCODE addRAcklist(pSipDlg *const pDlg,const char* rack);


/* beAck==1,if ack event from tx */
RCODE ProcessReq(pSipDlg *pdlg,TxStruct txData,int beAck,pSipDlg *rpdlg)
{
	pSipDlg dlg;
	SipReq req=NULL;
	SipRsp rsp=NULL;
	char *callid=NULL,*fromtag=NULL,*totag=NULL;
	char *contact=NULL;
	char buffer[DLGBUFFERSIZE]={'\0'};

	SipMethodType method;
	SipCSeq *pCseq=NULL;
	
	UserData *ud=NULL;
	DxLst routeset=NULL;

	/* RFC 3891 replaces */
	SipReplaces *rp;
	pSipDlg replaceddlg=NULL;

	char *reasonph=NULL;
	
	if( !pdlg){
		DlgPrintEx ( TRACE_LEVEL_API, "[ProcessReq] Invalid argument : Null pointer to dlgmsg" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if (beAck) 
		req=sipTxGetAck(txData);
	else
		req=sipTxGetOriginalReq(txData);
	if (!req) {
		DLGERR("[ProcessReq] Invalid argument : request is not found\n");
		return RC_ERROR;
	}
	method=GetReqMethod(req);
	callid=(char *)sipReqGetHdr(req,Call_ID);
	totag=GetReqToTag(req);
	fromtag=GetReqFromTag(req);
	/* if(sipDlgReqChk(req)!=RC_OK){ */

	reasonph = strDup(sipDlgReqChk(req));
	if(NULL!=reasonph){
		/* 400 bad request */
		if (method!=ACK){
			if(method != UNKNOWN_METHOD)
				/* sipDlgNewRsp(&rsp,req,"400","Bad Request - DialogLayer"); */
				sipDlgNewRsp(&rsp,req,"400",reasonph);
			else{
				/* add allow method */
				sipDlgNewRsp(&rsp,req,"405","Method Not Allow - DialogLayer");
				
				sprintf(buffer,"Allow:%s\0",ALLOW_METHOD);
				sipRspAddHdrFromTxt(rsp,Allow,buffer);
			}
			
			if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
				sipRspFree(rsp);
		}

		free(reasonph);

		DLGERR("[ProcessReq] Get a bad request\n");
		return RC_ERROR;
	}

	/* RFC 3891 replaces */
	rp=(SipReplaces *)sipReqGetHdr(req,Replaces);
	if(rp!=NULL){
		/* get replace dialog */
		replaceddlg=GetReplacedDlg(rp);
		if(NULL==replaceddlg){
			/* return 481 for this request */
			sipDlgNewRsp(&rsp,req,"481","Replace dialog is not existed - DialogLayer");
			
			if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
				sipRspFree(rsp);
			
			DLGERR("[ProcessReq] Get a bad request\n");
			return RC_ERROR;
		}else{
			/* if dialog terminated , send 603 back */
			if(DLG_STATE_TERMINATED==replaceddlg->state){
				sipDlgNewRsp(&rsp,req,"603","Declined (dialog terminated) - DialogLayer");
			
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
				
				DLGERR("[ProcessReq] Get a bad request\n");
				return RC_ERROR;
			}

			/* if "early-only" tag is presented, and call state is not early,
			 * send 486 back. 
			 */
			if(BeReplacesEarlyonly(req) && (DLG_STATE_EARLY!=replaceddlg->state)){
				sipDlgNewRsp(&rsp,req,"486","Busy(No early-only) - DialogLayer");
			
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
				
				DLGERR("[ProcessReq] Get a bad request\n");
				return RC_ERROR;
			}
	
			if(rpdlg)
				*rpdlg=replaceddlg;
		}
	}

	/* check option tags for TTCN */
	if((BYE==method)||(SIP_SUBSCRIBE==method)||(SIP_NOTIFY==method)||(REFER==method)){
		if(!CheckOptionTag(req)){
			sipDlgNewRsp(&rsp,req,"420","Bad Extension - DialogLayer");
			sipRspAddHdr(rsp,Unsupported,sipReqGetHdr(req,Require));
			
			if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
				sipRspFree(rsp);
	
			return RC_ERROR;
		}
	}

	/* check for Subscribe/Notify */
	if((method==SIP_SUBSCRIBE)||(method==SIP_NOTIFY)){
		if(sipReqGetHdr(req,Event)==NULL){
			/* 489 bad Event */
			sipDlgNewRsp(&rsp,req,"489","Bad Event - DialogLayer");
            if (NULL!=g_allowevent) 
				sprintf(buffer,"Allow-Events:%s\0",g_allowevent);
			else
				sprintf(buffer,"Allow-Events:%s\0",SUPPORTEVENT);
			sipRspAddHdrFromTxt(rsp,Allow_Events,buffer);
			if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
				sipRspFree(rsp);
			return RC_ERROR;
		}
		if(method==SIP_NOTIFY){
			if(sipReqGetHdr(req,Subscription_State)==NULL){
				sipDlgNewRsp(&rsp,req,"400","No Subscription State - DialogLayer");
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
				return RC_ERROR;
			}
		}
	}
	/* check for refer*/
	if (method==REFER) {
		SipReferTo *referto;
		referto=sipReqGetHdr(req,Refer_To);
		if (!referto 
			|| !referto->address 
			|| !referto->address->addr 
			|| referto->address->next
			||(referto->numAddr!=1)){
			
			sipDlgNewRsp(&rsp,req,"400","Refer-To URL is zero or more than one - DialogLayer");
			
			if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
				sipRspFree(rsp);
			return RC_ERROR;
		}	
	}

	if(dlg==NULL){
		/* match dialog in db */
		dlg=GetMatchDlg(method,callid,fromtag,totag);
	}
	
	/* if match and method = invite and subscribe ,add reference */
	/* no match dialog*/
	/* if totag is present, 481 else create dialog for invite or subscribe*/
	/* dialog creator : invite sip_subscribet*/
	if(!dlg){
		if(totag){
			if (method!=ACK) 
				if(sipDlgNewRsp(&rsp,req,"481","Dialog does not exist - DialogLayer")==RC_OK)
					if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
						sipRspFree(rsp);
			return RC_ERROR;
		}
		if((method==SIP_NOTIFY)||(method==BYE)||(method==CANCEL)){
			/* for in-dialog method,481 */
			DlgPrintEx ( TRACE_LEVEL_API, "[ProcessReq] Request must be in dialog,send 481!\n" );
			if(sipDlgNewRsp(&rsp,req,"481","Dialog does not exist - DialogLayer")==RC_OK)
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
			return RC_ERROR;
		}else if(BeCreateDlg(method)){
			char *remoteurl,*localurl;
			remoteurl=GetReqFromUrl(req);
			localurl=GetReqToUrl(req);

			/* already check in sipreqcheck function
			if( !remoteurl || !localurl ){
				if(sipDlgNewRsp(&rsp,req,"400","Bad Request - DialogLayer")==RC_OK)
					if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
						sipRspFree(rsp);
				return RC_ERROR;
			}
			*/
			if(RC_OK!=sipDlgNew(&dlg,callid,localurl,remoteurl,NULL)){
				DLGERR("memory allocate failed!");
				return RC_ERROR;
			}else{
				if(method==INVITE)
					sipDlgAddEvent(&dlg,INVITE_CALL);
				dlg->state=DLG_STATE_TRYING;
			}

			/* update creator method */
			if(UNKNOWN_METHOD==dlg->createmethod)
				dlg->createmethod=method;

		}else{
			/* out of dialog message */
			return RC_OK;
		}
	}

	/* for in-dialog handle */
	*pdlg=dlg;
	if(dlg->state==DLG_STATE_TERMINATED){
		DlgPrintEx ( TRACE_LEVEL_API, "[ProcessReq] DLG state is terminated,Send 481!\n" );
		/* 481 */
		if(ACK != method)
			if(sipDlgNewRsp(&rsp,req,"481","Dialog is terminated - DialogLayer")==RC_OK)
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
		return RC_ERROR;
	}


	/* SIMPLE message handle ,add reference */
	if(method==SIP_NOTIFY){
		SipSubState *substate=sipReqGetHdr(req,Subscription_State);
		if(substate){
			if(substate->state){
				if((strICmp(substate->state,"active")==0)||(strICmp(substate->state,"pending")==0)){
					sipDlgAddEvent(&dlg,GetReqEvent(req));
					dlg->state=DLG_STATE_CONFIRM;
				}
				if(strICmp(substate->state,"terminated")==0)
					sipDlgRemoveEvent(&dlg,GetReqEvent(req));
			}
		}else{
			if(sipDlgNewRsp(&rsp,req,"400","Subscription-state is not found - DialogLayer")==RC_OK)
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
			return RC_ERROR;
		}
	}

	if(dxLstGetSize(dlg->eventlist)==0){
		if(dlg->state==DLG_STATE_CONFIRM)
			dlg->state=DLG_STATE_TERMINATED;
	}
	
	/* update remote parameters of dialog */
	pCseq=sipReqGetHdr(req,CSeq);
	if(dlg->remotecseq < pCseq->seq)	
		dlg->remotecseq=pCseq->seq;
	else{
		if((CANCEL==method)||(ACK==method))
			;/* todo: should check cancel or ack match dlg->invitecseq or not */
		else{
			/* lower remoter cseq,shall return 500 */
			if(RC_OK==sipDlgNewRsp(&rsp,req,"500","Internal Server Error due to CSeq is lower - DialogLayer")){
				/* add re-try header */
				
				sipRspAddHdrFromTxt(rsp,Retry_After,"Retry-After:10\0");
				if(RC_OK!=sipDlgSendRspEx((void*)txData,rsp))
					sipRspFree(rsp);
			}
			return RC_ERROR;
		}
	}

	/* update RSeq if PRACK received */
	/* for prack */
	if(PRACK==method){
		dlg->rseq++;
	}

	if(NULL==dlg->remotetag)
		dlg->remotetag=strDup(fromtag);
	contact=GetReqContact(req);
	if(contact){
		if(dlg->remotetarget) 
			free(dlg->remotetarget);
		dlg->remotetarget=strDup(contact);
	}
	/* displayname */
	if(NULL==dlg->remotedisplayname)
		dlg->remotedisplayname=strDup(GetReqFromDisplayname(req));
	
	/* add route if record-route exists */
	if((dlg->routeset==NULL)&&(dlg->state==DLG_STATE_TRYING)){
		routeset=GetReqRecRoute(req);
		if(routeset)
			dlg->routeset=routeset;
	}

	/*add tx to dialog's txlist */
	if((sipTxGetUserData(txData)==NULL)&& (method!=ACK)){
		ud=(UserData*)calloc(1,sizeof(UserData));
		if( !ud ){
			DlgPrintEx ( TRACE_LEVEL_API, "[ProcessReq] Memory allocation error\n" );
			return RC_ERROR;
		}
		ud->usertype=txu_dlg;
		ud->data=(void*)dlg;
		sipTxSetUserData(txData,(void*)ud);
	}
	if (method!=ACK) 
		sipDlgAddTx(&dlg,txData);
	
	/* auto send 200 ok for BYE */
	if(method==BYE){
		if(dlg->state==DLG_STATE_CONFIRM){
			if(sipDlgNewRsp(&rsp,req,"200","ok - DialogLayer")==RC_OK){
				if(RC_OK!=sipDlgSendRsp(&dlg,rsp))
					sipRspFree(rsp);
			}else
				return RC_ERROR;
		}else{
			DlgPrintEx ( TRACE_LEVEL_API, "[ProcessReq] Dialog state is not confirm,send 481!\n" );
			if(sipDlgNewRsp(&rsp,req,"481","Dialog does not exist - DialogLayer")==RC_OK){
				if(RC_OK!=sipDlgSendRsp(&dlg,rsp))
					sipRspFree(rsp);
			}else
				return RC_ERROR;
		}
	}	
	
	return RC_OK;
}

RCODE ProcessRsp(TxStruct txData)
{
	pSipDlg dlg=NULL;
	SipReq req;
	SipRsp rsp=sipTxGetLatestRsp(txData);
	char *remotetarget,*totag,*remotetag;
	SipMethodType method;
	DxLst routeset=NULL;

	DlgState state;
	INT32 rspcode=999;
	UserData	*ud=NULL;
	

	ud=(UserData*)sipTxGetUserData(txData);
	if(ud){
		if(ud->usertype==txu_dlg)
			dlg=(pSipDlg)ud->data;
		else{
			return RC_OK;
		}
	}else{
		DlgPrintEx ( TRACE_LEVEL_API, "[ProcessRsp] Invalid argument : Null pointer to dlg\n" );
		return RC_ERROR;
	}
	
	if(!dlg){
		DlgPrintEx ( TRACE_LEVEL_API, "[ProcessRsp] Invalid argument : Null pointer to dlg\n" );
		return RC_ERROR;
	}
	if(!rsp){
		DlgPrintEx ( TRACE_LEVEL_API, "[ProcessRsp] Invalid argument : Null pointer to response\n" );
		return RC_ERROR;
	}

	/* check is rsp is ok */
	if(sipDlgRspChk(rsp)!=RC_OK){
		DLGERR (  "[ProcessRsp] Invalid SIP header in Sip Response" );
		return RC_ERROR;
	}
	
	/* update dialog remote parameter */
	req=sipTxGetOriginalReq(txData);
	if(!req){
		DLGWARN("[ProcessRsp] Invalid argument : Null pointer to original request\n" );
		/*return RC_OK;*/
		remotetarget=dlg->remotetag;
		method=GetRspMethod(rsp);
	}else{
		remotetag=GetReqToTag(req);
		method=GetReqMethod(req);
	}
	
	totag=GetRspToTag(rsp);
	sipDlgGetDlgState(&dlg,&state);
	
	/* check remotetag in dialog and msg */
	if( remotetag ) {
		if( totag ){
			if(strCmp(remotetag,totag)!=0){
				DLGWARN("[ProcessRsp] bad response:to-tag is not matched" );
				return RC_ERROR;
			}
		}else{
			/* if dialog if confirm ,it's not allowed */
			if(DLG_STATE_CONFIRM==state){
				DLGWARN("[ProcessRsp] bad response:to-tag is NULL" );
				return RC_ERROR;
			}
		}
	}else{
		/* remote tag == NULL */
		if(NULL!=totag){
			if (DLG_STATE_CONFIRM==state){

				/* because 200 ok of subscribe may 
				   be received after notify request 
				   so dialog state may be confirm
				*/
				if((SIP_SUBSCRIBE!=method)&&(REFER!=method)){
					DLGWARN("[ProcessRsp] bad response : remotetag is null but to-tag is not null!" );
					return RC_ERROR;
				}
			}	
		}
	}
	
	if(totag){
		if((!dlg->remotetag)&&BeCreateDlg(method))
			dlg->remotetag=strDup(totag);

		/* new situation : if dialog is in early state ,update tag by new response */
		if(DLG_STATE_EARLY==state){
			if(dlg->remotetag)
				free(dlg->remotetag);
			dlg->remotetag=strDup(totag);
		}	
	}

	/* remote target */
	remotetarget=GetRspContact(rsp);
	if(remotetarget){
		if(dlg->remotetarget)
			free(dlg->remotetarget);
		dlg->remotetarget=strDup(remotetarget);
	}

	/* displayname */
	if(NULL==dlg->remotedisplayname)
		dlg->remotedisplayname=strDup(GetRspToDisplayname(rsp));
	
	/* add route if record-route exists */
	if((dlg->routeset==NULL)&&BeCreateDlg(method)){
		routeset=GetRspRecRoute(rsp);
		
		if(routeset){
			if(dlg->bRR==TRUE){
				char *p=dxLstGetAt(routeset,0);
				if(NULL != p) free(p);
				dlg->bRR=FALSE;
			}
			dlg->routeset=routeset;
		}
	}

	/* dialog state machine */
	rspcode=GetRspStatusCode(rsp);
	if(BeDestroyDlg(rspcode)&&(state!=DLG_STATE_NULL)){
		dlg->state=DLG_STATE_TERMINATED;
		state = dlg->state;
	}
	if(BeCreateDlg(method)){
		if(( rspcode>=100)&&(rspcode<200)){
			if((DLG_STATE_TRYING==state))
				dlg->state=DLG_STATE_PROCEEDING;
			
			if((DLG_STATE_TRYING==state)||(DLG_STATE_PROCEEDING==state))
				if(NULL!=totag)
					dlg->state=DLG_STATE_EARLY;
		}

		if((DLG_STATE_TRYING==state)||(DLG_STATE_PROCEEDING==state)||(DLG_STATE_EARLY==state)){
			if(( rspcode>=200)&&(rspcode<300)){
				/*
				//dlg->state=DLG_STATE_CONFIRM;
				*/
				if(method==INVITE){
					dlg->state=DLG_STATE_CONFIRM;
					sipDlgAddEvent(&dlg,INVITE_CALL);
				}else{
					/*
					 * for subscribe and refer
					 * 2xx mean if event is pending or accepted and
					 * dialog is confirm ,too.
					 */
					char *event=GetReqEvent(req);
					if(event)
						sipDlgAddEvent(&dlg,event);
					
					dlg->state=DLG_STATE_CONFIRM;
				}
			}else if(rspcode>299){
				dlg->state=DLG_STATE_TERMINATED;
			}
			
		}		
	}

	/* to support RFC3262 PRACK */
	if(method==INVITE){
		/* check 100rel and RSeq */
		if(Be100rel(rsp)){
			int rseq=*(int *)sipRspGetHdr(rsp,RSeq); 
			/* add "RAck= CSeq + RSeq" to list */
			if(rseq > 0){
				char rackbuf[64];	
				SipCSeq *cseqhdr=(SipCSeq *)sipRspGetHdr(rsp,CSeq);
				
				memset(rackbuf,0,sizeof(rackbuf));
				sprintf(rackbuf,"%d %d %s",rseq,cseqhdr->seq,sipMethodTypeToName(cseqhdr->Method));
				if(RC_OK!=addRAcklist(&dlg,rackbuf))
					DLGWARN("add rack to sipDlg is failed!");
			}
		}/* end of 100rel */
	}

	if(method==BYE){
		if((rspcode != 401) && (rspcode !=407)){
		
			sipDlgRemoveEvent(&dlg,INVITE_CALL);
			if(dxLstGetSize(dlg->eventlist)==0)
				dlg->state=DLG_STATE_TERMINATED;
		}
	}
	
	return RC_OK;
}

RCODE ProcessNoMatchRSP(TxStruct txData)
{
	SipRsp	tmpRsp=NULL;
	SipMethodType method;
	pDlgMsg msg=NULL;

	int statuscode;

	tmpRsp=sipTxGetLatestRsp(txData);
	statuscode=GetRspStatusCode(tmpRsp);
	method=GetRspMethod(tmpRsp);
	
	if((INVITE==method)&&(statuscode>199)&&(statuscode<300)){
		/* find match dialog for this 200 ok */
		Entry dlglist;
		char *callid=NULL,*fromtag=NULL,*totag=NULL;
	
		pSipDlg tmpdlg=NULL;
		BOOL betotag;
		BOOL befromtag;
		
		callid=(char *)sipRspGetHdr(tmpRsp,Call_ID);
		fromtag=GetRspFromTag(tmpRsp);
		totag=GetRspToTag(tmpRsp);
		
		
		dlglist=dxHashItemLst(DLGDBLst,callid);
		while ((tmpdlg=(pSipDlg)dxEntryGetValue(dlglist))) {
			
			if(!strCmp(callid,tmpdlg->callid))
				;
			else{
				tmpdlg=NULL;
				dlglist=dxEntryGetNext(dlglist);
				continue;
			}
			
			betotag=BeMatchTag(totag,tmpdlg->remotetag);
			befromtag=BeMatchTag(fromtag,tmpdlg->localtag);

			if(befromtag && betotag)
				break;
			else
				tmpdlg=NULL;

				dlglist=dxEntryGetNext(dlglist);
		
		}

		/* todo: 200 ok with no dialog, should create a new dialog for it */
		
		/* update remote target and route */
		if(tmpdlg){
			DxLst routeset=NULL;
			char* remotetarget=GetRspContact(tmpRsp);
			if(remotetarget){
				if(tmpdlg->remotetarget)
					free(tmpdlg->remotetarget);
				tmpdlg->remotetarget=strDup(remotetarget);
			}
			
			/* FOR RFC 3261, route set should not be changed */
			/* FOR TTCN add route if record-route exists */
			routeset=GetRspRecRoute(tmpRsp);
			if(routeset){
				if(tmpdlg->bRR==TRUE){
					char *p=dxLstGetAt(routeset,0);
					if(NULL != p) free(p);
					tmpdlg->bRR=FALSE;
				}
				tmpdlg->routeset=routeset;
			}
			
			/* post to upper layer */
			if(sipDlgMsgNew(&msg,tmpdlg,NULL,tmpRsp,txData)== RC_OK){
				if(sipDlgPostMsg(DLG_MSG_RSP,msg)!= RC_OK)
					DLGERR("[TxEvtCB] post msg failed!");

				sipDlgMsgFree(&msg);
			}else
				DLGERR("[TxEvtCB] create post msg failed!");			
			return RC_OK;
		}
	}/* end of INVITE && 200~299 rsp */
		
	/* not invite or no successful rsp */
	return RC_ERROR;
}


RCODE 
sipDlgPostMsg(IN DlgEvtType event,IN pDlgMsg pMsg)
{

	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgPostMsg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	
	g_dlgevtcb(event,(void*)pMsg);

	return RC_OK;
}

void	AddStatusLine(SipReq request,SipRsp response)
{
	SipRspStatusLine *line=(SipRspStatusLine*)calloc(1,sizeof(SipRspStatusLine));
	
	line->ptrVersion=strDup("SIP/2.0");
	line->ptrStatusCode=strDup("200");
	line->ptrReason=strDup("OK");
	
	sipRspSetStatusLine(response,line);
	free(line->ptrReason);
	free(line->ptrStatusCode);
	free(line->ptrVersion);
	free(line);
}


void FillHeader(SipReq request,SipRsp response)
{
	sipRspAddHdr(response,From,sipReqGetHdr(request,From));
	sipRspAddHdr(response,To,sipReqGetHdr(request,To));
	sipRspAddHdr(response,Call_ID,sipReqGetHdr(request,Call_ID));
	sipRspAddHdr(response,CSeq,sipReqGetHdr(request,CSeq));
	sipRspAddHdr(response,Via,sipReqGetHdr(request,Via));
}

pSipDlg GetReplacedDlg(SipReplaces *replaces)
{
	pSipDlg ByeDlg=NULL;

	/* check replace if it exists */
	/* RFC 3891 replace*/
	if(replaces!=NULL){
		char *rcallid=replaces->callid;
		char *rtotag=NULL,*rfromtag=NULL;
		
		SipParam *rparam=replaces->paramList;

		/* get fromtag and totag from replace */
		while(rparam){
			if(strICmp(rparam->name,"from-tag")==0)
				rfromtag=rparam->value;
			else if (strICmp(rparam->name,"to-tag")==0)
				rtotag=rparam->value;
		
				rparam=rparam->next;
		}
		
		/* match transaction for replace */
		ByeDlg=GetMatchDlg(INVITE,rcallid,rfromtag,rtotag);
	}

	return ByeDlg;

}

void TxEvtCB(TxStruct txData, SipTxEvtType event)
{
	pSipDlg dlg=NULL; 
	SipReq  ack=NULL,tmpReq=NULL;
	SipRsp	tmpRsp=NULL;
	pDlgMsg msg=NULL;
	UserData *ud=NULL;
	SipMethodType method=sipTxGetMethod(txData);
	DlgEvtType postevent;

	pSipDlg rdlg=NULL;

	ud=(UserData*)sipTxGetUserData(txData);
	switch (event) {
		case TX_TRANSPORT_ERROR:
		case TX_TIMER_B:
		case TX_TIMER_F:
		case TX_TIMER_H:
			/* set dlg state to terminated */
			tmpReq=sipTxGetOriginalReq(txData);
			tmpRsp=sipTxGetLatestRsp(txData);
			if(ud){
				if(ud->usertype==txu_dlg){
					dlg=(pSipDlg)ud->data;
					if(dlg)
						dlg->state=DLG_STATE_TERMINATED;
				}
			}
			if(sipTxGetType(txData)==TX_CLIENT_2XX){
				postevent=DLG_2XX_EXPIRE;
				DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Not Receive ACK,so diconnect!\n");
			}else{
				if(TX_TRANSPORT_ERROR==event)
					postevent=DLG_MSG_TRANSPORTERROR;
				else
					postevent=DLG_MSG_TX_TIMEOUT;
			}
			
			/* post event message */
			if(sipDlgMsgNew(&msg,dlg,tmpReq,tmpRsp,(void*)txData)== RC_OK){
				if(sipDlgPostMsg(postevent,msg)!= RC_OK)
					DLGERR("[TxEvtCB] post msg failed!");;
			}else
				DLGERR("[TxEvtCB] create post msg failed!");
			break;
		case TX_TIMER_A:
		case TX_TIMER_E:
			break;
		case TX_REQ_RECEIVED:
			/*receive a request transaction*/
			tmpReq=sipTxGetOriginalReq(txData);

#ifdef ICL_NO_MATCH_NOTIFY
			if(tmpReq &&(GetReqMethod(tmpReq)==SIP_NOTIFY)){
				if(NULL==GetReqToTag(tmpReq)){
					SipRsp newrsp;
					newrsp=sipRspNew();
					if(newrsp){
						AddStatusLine(tmpReq,newrsp);
						FillHeader(tmpReq,newrsp);
						
					}
					/* post to upper layer */
					if(sipDlgMsgNew(&msg,NULL,tmpReq,newrsp,(void*)txData)== RC_OK)
						if(sipDlgPostMsg(DLG_MSG_NOMATCH_REQ,msg)!= RC_OK)
								DLGERR("[TxEvtCB] post msg failed!");

					sipTxSendRsp(txData,newrsp);

					break;
				}
			}
#endif /* ICL_NO_MATCH_NOTIFY */

			/* process request */
			if(ProcessReq(&dlg,txData,0,&rdlg)==RC_OK){	
				/* post message */
				if(sipDlgMsgNew(&msg,dlg,tmpReq,tmpRsp,txData)== RC_OK){
					if(method==ACK){
						if(sipDlgPostMsg(DLG_MSG_ACK,msg)!= RC_OK)
							DLGERR("[TxEvtCB] post msg failed!");
					}else{
						if(rdlg!=NULL)
							sipDlgMsgSetReplacedDlg(&msg,rdlg);
						if(sipDlgPostMsg(DLG_MSG_REQ,msg)!= RC_OK)
							DLGERR("[TxEvtCB] post msg failed!");
					}
				}else
					DLGERR("[TxEvtCB] create post msg failed!");
			}
			break;
		case TX_RSP_RECEIVED:
			if(ProcessRsp(txData)==RC_OK){
				tmpRsp=sipTxGetLatestRsp(txData);
				tmpReq=sipTxGetOriginalReq(txData);
				if(ud){
					if(ud->usertype==txu_dlg)
						dlg=(pSipDlg)ud->data;
				}
				/* post message */
				if(!dlg)
					DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Out Dialog response received!\n");
				if(GetRspMethod(tmpRsp)!=CANCEL){
					if(sipDlgMsgNew(&msg,dlg,tmpReq,tmpRsp,txData)== RC_OK){
						if(sipDlgPostMsg(DLG_MSG_RSP,msg)!= RC_OK)
							DLGERR("[TxEvtCB] post msg failed!");
					}else
						DLGERR("[TxEvtCB] create post msg failed!");
				}
				
			}
			break;
		case TX_CANCEL_RECEIVED:
			if(ud){
				if(ud->usertype==txu_dlg)
					dlg=(pSipDlg)ud->data;
			}else
				dlg=sipDlgMatchByTx(txData);

			tmpReq=sipTxGetOriginalReq(txData);
			if(sipDlgMsgNew(&msg,dlg,tmpReq,tmpRsp,txData)== RC_OK){
				if(sipDlgPostMsg(DLG_CANCEL_EVENT,msg)!= RC_OK)
					DLGERR("[TxEvtCB] post msg failed!");
			}else
					DLGERR("[TxEvtCB] create post msg failed!");
			break; 
		case TX_NOMATCH_RSP_RECEIVED :
			/* todo: should handle 200 ok for invite */
			ProcessNoMatchRSP(txData);
	
			ud=(UserData*)sipTxGetUserData(txData);
			if(NULL==ud)
				sipTxFree(txData);
			break;
		case TX_ACK_RECEIVED:
			/* process ack */
			/* update contact if it exists*/
			if(ProcessReq(&dlg,txData,1,NULL)==RC_OK){
				ack=sipTxGetAck(txData);
				tmpRsp=sipTxGetLatestRsp(txData);
				if (GetRspStatusCode(tmpRsp)>299) {
					DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] get a non-2xx ack!\n");
					
				}else{ 
				/* post message */
					if(sipDlgMsgNew(&msg,dlg,ack,tmpRsp,txData)== RC_OK){
						if(sipDlgPostMsg(DLG_MSG_ACK,msg)!= RC_OK)
							DLGERR("[TxEvtCB] post msg failed!\n");;
					}else
						DLGERR("[TxEvtCB] create post msg failed!\n");
				}
			}	
			
			ud=(UserData*)sipTxGetUserData(txData);
			if(NULL==ud)
				sipTxFree(txData);

			break;
		case TX_TERMINATED_EVENT:
			ud=(UserData*)sipTxGetUserData(txData);
			
			if(ud){
				if(ud->usertype==txu_dlg)
					dlg=(pSipDlg)ud->data;
				else
					dlg=NULL;
			}else{
				DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Free a SipTx with no UserData!\n");
				sipTxFree(txData);
				break;
			}
			if(dlg){
				/*if(dlg->state ==DLG_STATE_TERMINATED){
				 remove sipTx from dialog in any state...*/
					
				DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Free a SipTx in dialog!\n");
				sipDlgDelTx(&dlg,txData);
				dlgFreeUserData(ud);
				sipTxFree(txData);
					
			}else{
				/* post to dialog user */
				/* process response for NULL dialog */
				DlgPrintEx(TRACE_LEVEL_API,"[TxEvtCB] post non-dialog tx terminated!\n");
				if(sipDlgMsgNew(&msg,dlg,tmpReq,tmpRsp,txData)== RC_OK){
					if(sipDlgPostMsg(DLG_MSG_TX_TERMINATED,msg)!= RC_OK)
						DLGERR("[TxEvtCB] post msg failed!");
				}else
					DLGERR("[TxEvtCB] create post msg failed!");
			}
			break;
		default:
			break;
	}

	if(msg)
		sipDlgMsgFree(&msg);
}

/*Initiate Dialog Layer library */
CCLAPI 
RCODE 
sipDlgInit(IN DlgEvtCB eventCB,IN SipConfigData config)
{

	/* check if initialized or not */
	if (DLGDBLst) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgInit] dialog layer initiated!\n" );
		return RC_OK;
	}
	g_dlgevtcb=eventCB;
    
	if (g_allowevent) {
		free(g_allowevent);
		g_allowevent = NULL;
	}

	g_allowevent=strDup(SUPPORTEVENT);
	DLGDBLst=dxHashNew(DLGDBSIZE,0,DX_HASH_POINTER);
	if( !DLGDBLst ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgInit] dialog layer failed!\n" );
		return RC_ERROR;
	}
	
	if(RC_OK==sipTxLibInit(TxEvtCB,config,FALSE,SipTxCount)){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgInit] dialog layer initiated!\n" );
		return RC_OK;
	}else{
		if (g_allowevent) {
			free(g_allowevent);
			g_allowevent=NULL;
		}
		
		dxHashFree(DLGDBLst,NULL);
        DLGDBLst=NULL;
	
		DlgPrintEx ( TRACE_LEVEL_ERROR, "[sipDlgInit] to initiate sipTx is failed!\n" );
		return RC_ERROR;
	}
}

CCLAPI	
RCODE 
sipDlgClean(void)
{
/*	int pos,num;*/
	pSipDlg dlg;

	char *key;
	/* clean dialog database :DLGDBLst */
	if(DLGDBLst){
	
		StartKeys(DLGDBLst);
		while ((key=NextKeys(DLGDBLst))) {
			dlg=(pSipDlg)dxHashDel(DLGDBLst,key);
			sipDlgFree(&dlg);
		}
		TCRPrint ( TRACE_LEVEL_API, "[DLGLAYER] [sipDlgClean] total dialog:%d\n",dxHashSize(DLGDBLst) );
		dxHashFree(DLGDBLst,NULL);
        DLGDBLst=NULL;
	}

	if (g_allowevent) {
		free(g_allowevent);
		g_allowevent=NULL;
	}
	DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgClean] dialog layer cleaned!\n" );
	return sipTxLibEnd();
}

/* dispatch dialog events */
CCLAPI 
RCODE 
sipDlgDispatch(const INT32 timeout)
{
	return sipTxEvtDispatch(timeout);
}

/* create a  object */
CCLAPI 
RCODE 
sipDlgNew(pSipDlg *pdlg,const char *callid,const char *localurl,const char *remoteurl,DxLst routeset)
{
	pSipDlg dlg=NULL;
	/* check parameter */
	if ( !DLGDBLst) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNew] run sipDlgInit first\n" );
		return RC_ERROR;
	}
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNew] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if(callid==NULL){
		DlgPrintEx(TRACE_LEVEL_API,"[sipDlgNew] callid is NULL!!!\n");
		return RC_ERROR;
	}
	if(localurl==NULL){
		DlgPrintEx(TRACE_LEVEL_API,"[sipDlgNew] local-url is NULL!!!\n");
		return RC_ERROR;
	}
	if(remoteurl==NULL){
		DlgPrintEx(TRACE_LEVEL_API,"[sipDlgNew] remote-url is NULL!!!\n");
		return RC_ERROR;
	}
	
	/* alloca memory */
	dlg=(pSipDlg)calloc(1,sizeof(SipDlg));
	if ( ! dlg ) {
		DLGWARN("[sipDlgNew] Memory allocation error" );
		return RC_ERROR;
	}
	/* set initial value for dialog */
	dlg->callid=strDup(callid);
	dlg->localurl=strDup(localurl);
	dlg->localtag=taggen();
	dlg->localcseq=1;
	dlg->remoteurl=strDup(remoteurl);
	dlg->remotetag=NULL;
	dlg->remotetarget=strDup(remoteurl);
	dlg->remotecseq=0;
	
	/* dlg->secure=FALSE; set secure flag if remote url is sips */
	
	dlg->secure=BeSIPSURL(remoteurl);
	
	dlg->state=DLG_STATE_NULL;
	dlg->bRR=FALSE;

	dlg->rseq = rand();
	dlg->racklist = NULL;

	if(routeset)
		dlg->routeset=dxLstDup(routeset,NULL);
	else
		dlg->routeset=NULL;

	dlg->eventlist=NULL;
	dlg->txlist=NULL;

	/* support dialog event package */
	dlg->beInitiator=FALSE;
	dlg->dialogid=GetRand();

	/* set default method to UNKNOWN_METHOD */
	dlg->createmethod = UNKNOWN_METHOD;

	*pdlg=dlg;
	/* add to dialog list */
	dxHashAddEx(DLGDBLst,dlg->callid,(void*)dlg);
	DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgNew] dialog count:%d\n",dxHashSize(DLGDBLst));
	return RC_OK;
}

/* free a SipDlg object */
CCLAPI 
RCODE 
sipDlgFree(pSipDlg *pdlg)
{
	pSipDlg dlg;
	TxStruct tx;
	UserData* ud;
	INT32 pos,size;

	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgFree] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgFree] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!dlg->callid) {
		DLGWARN("callid is dlg is NULL!\n");
		return RC_ERROR;
	}
	/* check dialog state */
	if((dlg->state!=DLG_STATE_TERMINATED)&&(dlg->state!=DLG_STATE_NULL)){
		DlgPrintEx ( TRACE_LEVEL_API, "Wrong state of dialog in Free function!\n" );
	}


	/* remove from dialog db list */
	if (RC_OK!=dxHashDelEx(DLGDBLst,dlg->callid,(void*)dlg))
		DLGWARN("remove from dialog db list is failed!\n");
	
	/* free member of dialog */
	if(dlg->callid)
		free(dlg->callid);
	if(dlg->localurl)
		free(dlg->localurl);
	if(dlg->localtag)
		free(dlg->localtag);

	if(dlg->remotedisplayname)
		free(dlg->remotedisplayname);
	if(dlg->remoteurl)
		free(dlg->remoteurl);
	if(dlg->remotetag)
		free(dlg->remotetag);
	if(dlg->remotetarget)
		free(dlg->remotetarget);
	if(dlg->routeset)
		dxLstFree(dlg->routeset,NULL);
	if(dlg->eventlist)
		dxLstFree(dlg->eventlist,NULL);
	if(dlg->txlist){
		DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgFree] remote txlist,count:%d\n",dxLstGetSize(dlg->txlist));
		/*dxLstFree(dlg->txlist,(void (*)(void *)) sipTxFree);*/
		size=dxLstGetSize(dlg->txlist);
		for (pos=0;pos<size;pos++){
			tx=(TxStruct)dxLstGetAt(dlg->txlist,0);
			if (tx) {
				ud=(UserData*)sipTxGetUserData(tx);
				if (ud) dlgFreeUserData(ud);
				sipTxFree(tx);
			}
		}
		dxLstFree(dlg->txlist,NULL);
	}

	if(dlg->authreq)
		sipReqFree(dlg->authreq);

	if(dlg->racklist){
		dxLstFree(dlg->racklist,NULL);
	}

	/* set '0' for this memory block */
	memset(dlg,0,sizeof(SipDlg));
	free(dlg);
	DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgFree] total dialog count:%d\n",dxHashSize(DLGDBLst));
	/* set NULL for this point */
	*pdlg=NULL;
	return RC_OK;
}

/* get callid of this dialog */
CCLAPI
RCODE 
sipDlgGetCallID(pSipDlg *const pdlg,char *callid ,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetCllID] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetCllID] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!dlg->callid) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetCllID] Invalid argument : Null pointer to dialog's callid\n" );
		return RC_ERROR;
	}

	if(!callid||!len||(*len < 0)){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetCllID] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}
	if(*len==0){
		*len=strlen(dlg->callid);
		return RC_ERROR;
	}
	
	/* clean buffer */
	INT32 length = strlen(dlg->callid);
	memset(callid,0,(INT32)*len);
	/* if buffer size is enough ,copy callid to buffer */
	if(length< (INT32)*len ){
		strncpy(callid,dlg->callid, length);
		*len=length;
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Local url of this dialog */
CCLAPI
RCODE 
sipDlgGetLocalURL(pSipDlg *const pdlg,char *localurl,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !dlg->localurl ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalURL] Invalid argument : Null pointer to dialog's localurl\n" );
		return RC_ERROR;
	}
	if(!localurl||!len||(*len < 0)){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalURL] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}
	if(*len==0){
		*len=strlen(dlg->localurl);
		return RC_ERROR;
	}

	/* clean buffer */
	memset(localurl,0,sizeof(localurl));
	/* if buffer size is enough ,copy local url to buffer */
	if(strlen(dlg->localurl)< (unsigned int)*len ){
		strcpy(localurl,dlg->localurl);
		*len=strlen(dlg->localurl);
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Local tag of this dialog */
CCLAPI
RCODE 
sipDlgGetLocalTag(pSipDlg *const pdlg,char *localtag,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalTag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalTag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	if( !localtag || !len||(*len < 0) ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalTag] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}

	if(!dlg->localtag){
		*len=0;
		return RC_ERROR;
	}
	
	if(*len==0){
		*len=strlen(dlg->localtag);
		return RC_ERROR;
	}
	
	/* clean buffer */
	memset(localtag,0,sizeof(localtag));
	/* if buffer size is enough ,copy local tag to buffer */
	if(strlen(dlg->localtag)< (unsigned int)*len ){
		strcpy(localtag,dlg->localtag);
		*len=strlen(dlg->localtag);
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Local command sequence of this dialog */
CCLAPI
RCODE 
sipDlgGetLocalCseq(pSipDlg *const pdlg,UINT32 *lcseq)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if(!lcseq) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer\n" );
		return RC_ERROR;
	}
	/* set value */
	*lcseq=dlg->localcseq;
	return RC_OK;
}

/* set Local command sequence of this dialog */
CCLAPI 
RCODE 
sipDlgSetLocalCseq(IN pSipDlg *const pdlg,IN UINT32 lcseq)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	
	if(dlg->localcseq >= lcseq){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetLocalCseq] Invalid argument : new cseq is too small\n" );
		return RC_ERROR;
	}

	dlg->localcseq = lcseq;
	return RC_OK;
}

/* get Remote url of this dialog */
CCLAPI
RCODE 
sipDlgGetRemoteURL(pSipDlg *const pdlg,char *remoteurl,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !dlg->remoteurl ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog's remote url\n" );
		return RC_ERROR;
	}
	if(!remoteurl || !len || (*len < 0)){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}
	if(*len==0){
		*len=strlen(dlg->remoteurl)+1;
		return RC_ERROR;
	}
	/* clean buffer */
	memset(remoteurl,0,sizeof(remoteurl));
	/* if buffer size is enough ,copy remote url to buffer */
	if(strlen(dlg->remoteurl)< (unsigned int)*len ){
		strcpy(remoteurl,dlg->remoteurl);
		*len=strlen(dlg->remoteurl);
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Remote Tag of this dialog */
CCLAPI
RCODE 
sipDlgGetRemoteTag(pSipDlg *const pdlg,char *remotetag,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteTag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteTag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	
	if(!remotetag||!len||(*len < 0))
		return RC_ERROR;
	
	if(!dlg->remotetag){
		*len=0;
		return RC_ERROR;
	}
	if(*len==0){
		*len=strlen(dlg->remotetag);
		return RC_ERROR;
	}

	/* clean buffer */
	memset(remotetag,0,sizeof(remotetag));
	/* if buffer size is enough ,copy remote tag to buffer */
	if(strlen(dlg->remotetag)<= (unsigned int)*len ){
		strcpy(remotetag,dlg->remotetag);
		*len=strlen(dlg->remotetag);
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Remote target(contact address) of this dialog */
CCLAPI
RCODE 
sipDlgGetRemoteTarget(pSipDlg *const pdlg,char *remotetarget,INT32 *len)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !dlg->remotetarget ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to dialog's remote target\n" );
		return RC_ERROR;
	}
	if(!remotetarget||!len||(*len < 0)){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRemoteURL] Invalid argument : Null pointer to buffer or invalid buffer length\n" );
		return RC_ERROR;
	}
	if(*len==0){
		*len=strlen(dlg->remotetarget);
		return RC_ERROR;
	}

	/* clean buffer */
	memset(remotetarget,0,sizeof(remotetarget));
	/* if buffer size is enough ,copy remote target to buffer */
	if(strlen(dlg->remotetarget)< (unsigned int)*len ){
		strcpy(remotetarget,dlg->remotetarget);
		*len=strlen(dlg->remotetarget);
		return RC_OK;
	}else
		return RC_ERROR;
}

/* get Remote command sequence of this dialog */
CCLAPI
RCODE 
sipDlgGetRemoteCseq(pSipDlg *const pdlg,INT32 *rcseq)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!rcseq) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetLocalCseq] Invalid argument : Null pointer to cseq\n" );
		return RC_ERROR;
	}
	*rcseq=dlg->remotecseq;
	return RC_OK;
}

/* get Route Set of this dialog */
CCLAPI
RCODE 
sipDlgGetRouteSet(pSipDlg *const pdlg,DxLst* routeset)
{
	pSipDlg dlg;

	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRouteSet] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRouteSet] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if(!routeset){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetRouteSet] Invalid argument : Null pointer to route set\n" );
		return RC_ERROR;
	}
	
	/* copy list of routeset in dialog to out going list*/
	if(dlg->routeset){
		*routeset=dxLstDup(dlg->routeset,NULL);
	}else
		*routeset=NULL;
	return RC_OK;
}

/* set Route Set route set of this dialog */
CCLAPI
RCODE 
sipDlgSetRouteSet(pSipDlg *const pdlg,DxLst routeset)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetRouteSet] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetRouteSet] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	/* remote old route set */
	if(dlg->routeset){
		dxLstFree(dlg->routeset,(void (*)(void *))free);
		dlg->routeset=NULL;
	}
	/* set route set in dialog*/
	if(routeset)
		dlg->routeset=dxLstDup(routeset,NULL);

	return RC_OK;
}

/* get state of this dialog */
CCLAPI
RCODE 
sipDlgGetDlgState(pSipDlg *const pdlg,DlgState *state)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetDlgState] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetDlgState] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if ( !state ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetDlgState] Invalid argument : Null pointer to state\n" );
		return RC_ERROR;
	}

	*state=dlg->state;
	return RC_OK;

}

CCLAPI 
BOOL 
beSipDlgTerminated(IN pSipDlg *const pdlg)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[beSipDlgTerminated] Invalid argument : Null pointer to dialog\n" );
		return FALSE;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[beSipDlgTerminated] Invalid argument : Null pointer to dialog\n" );
		return FALSE;
	}

	if(DLG_STATE_TERMINATED == dlg->state)
		return TRUE;
	else
		return FALSE;
}

/* get secure flag of this dialog */
CCLAPI
RCODE 
sipDlgGetSecureFlag(pSipDlg *const pdlg,BOOL *secureflag)
{
	pSipDlg dlg;
	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetSecureFlag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetSecureFlag] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!secureflag) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetSecureFlag] Invalid argument : Null pointer to secureflag\n" );
		return RC_ERROR;
	}
	*secureflag=dlg->secure;
	return RC_OK;

}

/* use Dialog to Send a Request */
CCLAPI
RCODE 
sipDlgSendReq(pSipDlg *const pDlg,SipReq req)
{
	return sipDlgSendReqEx(pDlg,req,NULL);
}

CCLAPI 
RCODE 
sipDlgSendReqEx(IN pSipDlg *const pDlg,IN SipReq req,IN UserProfile up)
{
	pSipDlg dlg;
	TxStruct tx;
	SipMethodType method;
	UserData *ud;
	TxStruct inviteTx=NULL;
	/* SIMPLE message handle */
	SipSubState *substate=NULL;
	SipEvent	*event=NULL;
	
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	if( !req ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] Invalid argument : Null pointer to Sip Request\n" );
		return RC_ERROR;
	}
	/* check parameters in request message and store some information */
	if(DLG_STATE_TERMINATED==dlg->state){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] dialog is terminated!\n" );
		return RC_ERROR;
	}

	method=GetReqMethod(req);
	if(BeCreateDlg(method))
		if ((DLG_STATE_TRYING==dlg->state)||(DLG_STATE_PROCEEDING==dlg->state)){
			if(NULL==dlg->remotetag){
				DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] original request is not answered\n" );
				return RC_ERROR;
			}
	}

	/* check for simple message*/
	if (method==SIP_SUBSCRIBE) {
		event=(SipEvent	*)sipReqGetHdr(req,Event);
		if (!event || !event->pack) {
			DLGERR("[sipDlgSendReq] Bad subscribe!\n");
			return RC_ERROR;
		}
	}

	if (method==SIP_NOTIFY) {
		substate=(SipSubState *)sipReqGetHdr(req,Subscription_State);
		event=(SipEvent	*)sipReqGetHdr(req,Event);
		if(!substate || ! event){
			DLGERR("[sipDlgSendReq] Bad notify!\n");
			return RC_ERROR;
		}
		if(!substate->state || !event->pack){
			DLGERR("[sipDlgSendReq] Bad notify!\n");
			return RC_ERROR;
		}
	}
	
	/* if cancel ,must find a pending invitetx,and use its branchid */
	if(method==CANCEL) {
		int num,pos;
		num=dxLstGetSize(dlg->txlist);
		for(pos=0;pos<num;pos++){
			inviteTx=(TxStruct)dxLstPeek(dlg->txlist,pos);
			if(req){
				if((sipTxGetState(inviteTx) <TX_COMPLETED)&&(sipTxGetMethod(inviteTx)==INVITE))
					break;
				else
					inviteTx=NULL;
			}else
				inviteTx=NULL;
		}
		if(!inviteTx){
			DLGWARN("[sipDlgSendReq] progressing invite Tx is not found!\n");
			return RC_ERROR;
		}
	}

	/* create Tx to send request */
	ud=(UserData*)calloc(1,sizeof(UserData));
	if( !ud ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendReq] Invalid argument : Memory allocation error\n" );
		return RC_ERROR;
	}
	ud->usertype=txu_dlg;
	ud->data=(void*)dlg;

	if(inviteTx)
		tx=sipTxClientNew(req,(char*)sipTxGetTxid(inviteTx),up,(void*)ud);
	else	
		tx=sipTxClientNew(req,NULL,up,(void*)ud);

	/*tx=sipTxClientNew(req,NULL,NULL,(void*)ud);*/
	if(!tx){
		DLGERR("[sipDlgSendReq] create Tx is failed!\n");
		return RC_ERROR;
	}
	
	/* add to dialog txlist ;except ack transaction ;except ACK */
	if (method==ACK) {
		sipTxFree(tx);
		dlgFreeUserData(ud);
	}else{
		if(sipDlgAddTx(&dlg,tx)!=RC_OK){
			DLGWARN("[sipDlgSendReq] add Txlist is failed!\n");
			return RC_ERROR;
		}
	}
	
	/*
	 * for dialog creator ,add reference of dilaog 
	 * NOTIFY(with subscription-state=active)
	 */ 
	if((method==INVITE)||(method==SIP_SUBSCRIBE)||(method==REFER)){
		if(dlg->state == DLG_STATE_NULL) 
			dlg->state=DLG_STATE_TRYING;

		/* store authorization data for ack */
		if((INVITE==method)&&(sipReqGetHdr(req,Authorization)||sipReqGetHdr(req,Proxy_Authorization))){
			if(dlg->authreq!=NULL)
				sipReqFree(dlg->authreq);
			dlg->authreq=sipReqDup(req);
		}
	}else if (method==SIP_NOTIFY) {
		if((strICmp(substate->state,"active")==0)||(strICmp(substate->state,"pending")==0))
				sipDlgAddEvent(&dlg,GetReqEvent(req));
	}

#ifdef _DEBUG
		DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgSendReq] send a request\n%s\n",sipReqPrint(req) );
#else
		DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgSendReq] send a (%s)\n",PrintRequestline(req) );
#endif
		
	return RC_OK;
}

/* use Dialog to Send a Response */
CCLAPI
RCODE 
sipDlgSendRsp(pSipDlg *const pDlg,SipRsp rsp)
{
	pSipDlg dlg;
	TxStruct _tx=NULL,ptx;
	SipReq req;
	INT32 num,pos,rspcode;
	char *callid,*totag,*reqviabranch,*rspviabranch;
	RCODE rc;
	DlgState state;
	SipMethodType method;
	
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	/* check paramters in request message and store some information */
	/*if(dlg->state==DLG_STATE_TERMINATED){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] dialog is terminated\n" );
		return RC_ERROR;
	}*/

	if( !rsp ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] Invalid argument : Null pointer to Sip Response\n" );
		return RC_ERROR;
	}
	/* callid compare*/
	callid=(char*)sipRspGetHdr(rsp,Call_ID);
	if(strCmp(callid,dlg->callid)!=0){
			DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] Call ID is not match!\n" );
			return RC_ERROR;	
	}
	
	/* update or ckeck local tag */
	/* add to-tag */
	if(GetRspToTag(rsp)){
		if(!dlg->localtag)
			dlg->localtag=strDup(GetRspToTag(rsp));
	}else{
		if (!dlg->localtag){ 
			/*
			AddRspToTag(rsp,(const char*)taggen());
			dlg->localtag=strDup(GetRspToTag(rsp));
			*/
			dlg->localtag=taggen();
			AddRspToTag(rsp,dlg->localtag);
		}else
			AddRspToTag(rsp,dlg->localtag);	
	}

	if(strCmp(dlg->localtag,GetRspToTag(rsp))!=0){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] local tag is not match!\n" );
		return RC_ERROR;
	}

	/* for prack */
	/* to support RFC3262 PRACK */
	if(Be100rel(rsp))
		AddRspRSeq(rsp,dlg->rseq);

	/* match top via branch */
	rspviabranch=GetRspViaBranch(rsp);
	if (!rspviabranch) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] local tag is not match!\n" );
		return RC_ERROR;
	}
	num=dxLstGetSize(dlg->txlist);
	for(pos=0;pos<num;pos++){
		ptx=(TxStruct)dxLstPeek(dlg->txlist,pos);
		req=sipTxGetOriginalReq(ptx);
		if(req){
			reqviabranch=GetReqViaBranch(req);
			if(strCmp(reqviabranch,rspviabranch)==0){
				_tx=ptx;
				break;
			}
		}
	}
	if(_tx)
		rc=sipDlgSendRspEx((void*)_tx,rsp);
	else{
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRsp] transaction is not found!\n" );
		return RC_ERROR;
	}
	if(rc==RC_OK){
		/* update dialog tag*/
		if(!dlg->localtag){
			totag=GetRspToTag(rsp);
			if(totag)
				dlg->localtag=strDup(totag);
		}
		/* update dialog state */
		method=GetRspMethod(rsp);
		rspcode=GetRspStatusCode(rsp);
		sipDlgGetDlgState(pDlg,&state);
		if((( rspcode==481)||(rspcode==487)||(rspcode==408))&&(state!=DLG_STATE_NULL))
			dlg->state=DLG_STATE_TERMINATED;
		if((method==INVITE)||(method==SIP_SUBSCRIBE)||(method==REFER)){
			if(((DLG_STATE_TRYING==state)||(DLG_STATE_PROCEEDING==state)) && ( rspcode>=100)&&(rspcode<200))
				dlg->state=DLG_STATE_EARLY;
			if(((DLG_STATE_TRYING==state)||(DLG_STATE_PROCEEDING==state)||(DLG_STATE_EARLY==state)) && ( rspcode>=200)&&(rspcode<300)){
				dlg->state=DLG_STATE_CONFIRM;
				if(method==INVITE)
					sipDlgAddEvent(&dlg,INVITE_CALL);
			}
			if((state!=DLG_STATE_CONFIRM)&&(rspcode>=300))
				dlg->state=DLG_STATE_TERMINATED;
		}
	}
	if(method==BYE){
		if(dlg->state==DLG_STATE_CONFIRM){
			sipDlgRemoveEvent(&dlg,INVITE_CALL);
			if(dxLstGetSize(dlg->eventlist)==0)
				dlg->state=DLG_STATE_TERMINATED;
		}
	}
	return rc;

}

/* use Dialog to Send a Response */
CCLAPI
RCODE 
sipDlgSendRspEx(IN void *handle,IN SipRsp rsp)
{
	TxStruct _tx;
	INT32 rspcode;
	
	if( !handle ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRspEx] Invalid argument : Null pointer to handle\n" );
		return RC_ERROR;
	}
	if( !rsp ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSendRspEx] Invalid argument : Null pointer to Sip Response\n" );
		return RC_ERROR;
	}
	rspcode=GetRspStatusCode(rsp);
	_tx=(TxStruct)handle;

	if (sipTxGetState(_tx)>=TX_COMPLETED) {
		DLGWARN("[sipDlgSendRspEx] tx transaction is completed or terminated!\n");
		return RC_ERROR;
	}

	sipTxSendRsp(_tx,rsp);
	
#ifdef _DEBUG
	DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgSendRspEx] send a response\n%s\n",sipRspPrint(rsp) );
#else
	DlgPrintEx2 ( TRACE_LEVEL_API, "[sipDlgSendRspEx] send a (%s) to %s\n",PrintStatusline(rsp),GetRspViaSentBy(rsp));
#endif

  return RC_OK;
}

/* use Dialog to create a Request */
CCLAPI
RCODE 
sipDlgNewReq(pSipDlg *const pdlg,SipReq *preq,SipMethodType method)
{
	pSipDlg dlg;
	char buf[DLGBUFFERSIZE]={'\0'};
	SipReqLine *pReqline=NULL;
	SipReq req=NULL;
	INT32 route_size,i;

	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewReq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewReq] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !preq ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewReq] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}

	if( method >= UNKNOWN_METHOD){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewReq] method is unknwon \n" );
		return RC_ERROR;
	}

	/* first sip request must be dialog creator */
	if(UNKNOWN_METHOD==dlg->createmethod){
		if(BeCreateDlg(method)){
			/* update method of dlg if it's unknow */
			dlg->createmethod=method;
		}else{
			DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewReq] first request must be SIP INVITE/REFER/SUBSCRIBE\n" );
			return RC_ERROR;
		}
	}


	req=sipReqNew();
	if( !req ){
		DLGWARN("[sipDlgNewReq] Memory allocation error\n" );
		return RC_ERROR;
	}
	/* request line */
	pReqline=(SipReqLine*)calloc(1,sizeof(SipReqLine));
	if( !pReqline ){
		DLGWARN("[sipDlgNewReq] Memory allocation error\n" );
		sipReqFree(req);
		return RC_ERROR;
	}
	pReqline->iMethod=method;
	
	if(dlg->remotetarget&&(method!=CANCEL))
		pReqline->ptrRequestURI=strDup(dlg->remotetarget);
	else
		pReqline->ptrRequestURI=strDup(dlg->remoteurl);

	pReqline->ptrSipVersion=strDup(DLG_SIP_VERSION);
	
	if(sipReqSetReqLine(req,pReqline) != RC_OK){
		DLGERR("[sipDlgNewReq] To set request line is failed!\n");
		sipReqFree(req);
		sipReqLineFree(pReqline);
		return RC_ERROR;
	}
	sipReqLineFree(pReqline);
	
	/* add call id */
	if(dlg->callid){
		sprintf(buf,"Call-ID:%s\r\n",dlg->callid);
		if(sipReqAddHdrFromTxt(req,Call_ID,buf)!=RC_OK)
			DLGWARN("[sipDlgNewReq] To add siphdr callid failed\n" );
	}

	if(method == INVITE)
		dlg->invitecseq = dlg->localcseq;

	if((method != CANCEL)&&(method != ACK))
		sprintf(buf,"CSeq:%ud %s\r\n",dlg->localcseq++,sipMethodTypeToName(method));
	else
		sprintf(buf,"CSeq:%ud %s\r\n",dlg->invitecseq,sipMethodTypeToName(method));

	if(sipReqAddHdrFromTxt(req,CSeq,buf)!=RC_OK)
		DLGWARN("[sipDlgNewReq] To add siphdr cseq failed\n" );

	/* add from */
	if(dlg->localurl){
		if(dlg->localtag==NULL)
			dlg->localtag=taggen();
		sprintf(buf,"From:<%s>;tag=%s",dlg->localurl,dlg->localtag);
		if(sipReqAddHdrFromTxt(req,From,buf)!=RC_OK)
			DLGWARN("[sipDlgNewReq] To add siphdr from failed\n" );
	}

	/* add max-forward */
	sprintf(buf,"Max-Forwards:%d",SIP_MAX_FORWARDS);
	if(sipReqAddHdrFromTxt(req,Max_Forwards,buf)!=RC_OK)
		DLGWARN("[sipDlgNewReq] To add siphdr Max_Forwards failed\n" );

	/* add to */
	if(dlg->remoteurl){
		if(dlg->remotetag && (method!=CANCEL))
			sprintf(buf,"To:<%s>;tag=%s",dlg->remoteurl,dlg->remotetag);
		else
			sprintf(buf,"To:<%s>",dlg->remoteurl);
		
		if(sipReqAddHdrFromTxt(req,To,buf)!=RC_OK)
			DLGWARN("[sipDlgNewReq] To add siphdr to failed\n" );

		/* set displayname if it exists */
		if(NULL!=dlg->remotedisplayname){
			SipTo *pTo=NULL;

			pTo=(SipTo*)sipReqGetHdr(req,To);
			if((NULL != pTo) && (NULL!= pTo->address))
				pTo->address->display_name=strDup(dlg->remotedisplayname);
		}
	}
	
	/* add route */
	if(dlg->routeset){
		route_size=dxLstGetSize(dlg->routeset);
		for(i=0;i<route_size;i++){
			sprintf(buf,"Route:<%s>",(char*)dxLstPeek(dlg->routeset,i));
			if(sipReqAddHdrFromTxt(req,Route,buf)!=RC_OK)
				DLGWARN("[sipDlgNewReq] To add siphdr route failed\n" );
		}
	}else{
		if(CANCEL==method){
			TxStruct invitetx=NULL;
			SipReq oldinvite=NULL;
			
			sipDlgGetInviteTx(&dlg,FALSE,&invitetx);
			if(invitetx){
				oldinvite=sipTxGetOriginalReq(invitetx);
				if(oldinvite)
					sipReqAddHdr(req,Route,sipReqGetHdr(oldinvite,Route));
			}
		}
	}

	/* for ack or cancel */
	if((dlg->authreq!=NULL)&&((ACK==method)||(CANCEL==method))){
		/* add authorization header */
		sipReqAddHdr(req,Authorization,sipReqGetHdr(dlg->authreq,Authorization));
		/* add proxy-authorization header */
		sipReqAddHdr(req,Proxy_Authorization,sipReqGetHdr(dlg->authreq,Proxy_Authorization));
	}

	/* support RFC3262 PRACK */
	if((PRACK==method)&&(NULL!=dlg->racklist)){
		/* add RAck header to sip request */
		char *rackhdr=(char *)dxLstGetAt(dlg->racklist,0);

		if(NULL!=rackhdr){
			sprintf(buf,"RAck:%s",rackhdr);
			if(sipReqAddHdrFromTxt(req,RAck,buf)!=RC_OK)
				DLGERR("[sipDlgNewReq] To add siphdr RAck failed\n" );
			free(rackhdr);
		}	
	}
	
	/* add user agent */
	sprintf(buf,"User-Agent:%s\r\n\0",SIP_USER_AGENT);
	if(sipReqAddHdrFromTxt(req,User_Agent,buf)!=RC_OK)
		DLGWARN("[sipDlgNewReq] To add siphdr callid failed\n" );
	

	*preq=req;
	return RC_OK;
}

/* use Dialog to create a Response */
CCLAPI
RCODE 
sipDlgNewRsp(SipRsp* prsp,SipReq req,const char *statuscode,const char *phase)
{
	char	*callid=NULL,*fromtag=NULL;
	int istatuscode;
	char buf[256];

	SipRsp rsp=NULL;
	SipRspStatusLine *pStatusline=NULL;	
	SipCSeq *pCseq=NULL;
	SipFrom *pFrom=NULL;
	SipTo	*pTo=NULL;
	SipRecordRoute *pRecroute=NULL;
	SipVia *pVia=NULL;

	/* check parameter */
	if ( !prsp ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNewRsp] Invalid argument : Null pointer to prep\n" );
		return RC_ERROR;
	}
	if ( ! req ) {
		DLGERR ("[sipDlgNewRsp] Invalid argument : Null pointer to req\n" );
		return RC_ERROR;
	}
	
	/* check callid */
	callid=(char*)sipReqGetHdr(req,Call_ID);
	if (!callid) {
		DLGWARN ("[sipDlgNewRsp] callid is NULL\n" );
		/*return RC_ERROR;*/
	}
	/* check remote tag */
	fromtag=GetReqFromTag(req);
	if( !fromtag ){
		DLGWARN ("[sipDlgNewRsp] from or from-tag is NULL\n" );
		/*return RC_ERROR;*/
	}
	/* check to header */
	pTo=(SipTo*)sipReqGetHdr(req,To);
	if( !pTo ){
		DLGWARN ("[sipDlgNewRsp] to is NULL\n" );
		/*return RC_ERROR;*/
	}
	/* check cseq */
	pCseq=(SipCSeq*)sipReqGetHdr(req,CSeq);
	if(! pCseq){
		DLGWARN ("[sipDlgNewRsp] CSeq is NULL\n" );
		/*return RC_ERROR;*/
	}
	/* update remote parameter */
	/*
	if( !dlg->remotetag )
		dlg->remotetag=strDup(fromtag);
	
	contact=(char*)sipReqGetHdr(req,Call_ID);
	if(contact){
		if(dlg->remotetarget)
			free(dlg->remotetarget);
		dlg->remotetarget=strDup(contact);
	}
	*/
	/* construct response */
	rsp=sipRspNew();
	if( !rsp ){
		DLGERR("[sipDlgNewRsp] Memory allocation error\n" );
		return RC_ERROR;
	}
	pStatusline=calloc(1,sizeof(SipRspStatusLine));
	if( !pStatusline ){
		DLGERR("[sipDlgNewRsp] Memory allocation error\n" );
		sipRspFree(rsp);
		return RC_ERROR;
	}
	
	if(phase)
		pStatusline->ptrReason=strDup(phase);
	else
		pStatusline->ptrReason=strDup("OK");

	if(statuscode)
		pStatusline->ptrStatusCode=strDup(statuscode);
	else
		pStatusline->ptrStatusCode=strDup("200");

	pStatusline->ptrVersion=strDup(DLG_SIP_VERSION);
	if( sipRspSetStatusLine(rsp,pStatusline) != RC_OK ){
		DLGERR("[sipDlgNewRsp] To set request line is failed!\n");
		sipRspFree(rsp);
		sipRspStatusLineFree(pStatusline);
		return RC_ERROR;
	}
	sipRspStatusLineFree(pStatusline);
	
	/* add callid*/
	if(!callid || (sipRspAddHdr(rsp,Call_ID,(void*)callid) != RC_OK))
		DLGWARN("[sipDlgNewRsp] To add siphdr callid failed\n");
	
	/* add cseq */
	if(!pCseq || (sipRspAddHdr(rsp,CSeq,(void*)pCseq) != RC_OK))
		DLGWARN("[sipDlgNewRsp] To add siphdr cseq failed\n");

	/* add from */
	pFrom=(SipFrom*)sipReqGetHdr(req,From);
	if( !pFrom || (sipRspAddHdr(rsp,From,(void*)pFrom) != RC_OK))
		DLGWARN("[sipDlgNewRsp] To add siphdr from failed\n");

	/* add to */
	if( !pTo || (sipRspAddHdr(rsp,To,(void*)pTo) !=RC_OK))
		DLGWARN("[sipDlgNewRsp] To add siphdr to failed\n");
	
	/* copy record-route */
	istatuscode=a2i(statuscode);
	if((istatuscode>179)&&(istatuscode<300)){
		pRecroute=sipReqGetHdr(req,Record_Route);
		if(pRecroute)
			if(sipRspAddHdr(rsp,Record_Route,(void*)pRecroute) != RC_OK)
				DLGWARN("[sipDlgNewRsp] To add siphdr from failed\n");
	}

	
	/*copy Via*/
	pVia=(SipVia*)sipReqGetHdr(req,Via);
	if(pVia)
		sipRspAddHdr(rsp,Via,pVia);

	/* add user agent */
	sprintf(buf,"User-Agent:%s\r\n\0",SIP_USER_AGENT);
	if(sipRspAddHdrFromTxt(rsp,User_Agent,buf)!=RC_OK)
		DLGWARN("[sipDlgNewReq] To add siphdr callid failed\n" );

	*prsp=rsp;
	return RC_OK;
}

CCLAPI
RCODE 
sipDlgGetInviteTx(IN pSipDlg *const pdlg,IN BOOL beServerTx,OUT TxStruct *pTx)
{
	pSipDlg dlg;
	TxStruct tx;
	TXTYPE txtype;
	TXTYPE searchtype=TX_SERVER_INVITE;
	TXSTATE txstate;
	INT32 num,pos;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!pTx) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to TxStruct\n" );
		return RC_ERROR;
	}

	if(!beServerTx) searchtype=TX_CLIENT_INVITE;

	num=dxLstGetSize(dlg->txlist);
	for(pos=0;pos<num;pos++){
		tx=(TxStruct)dxLstPeek(dlg->txlist,pos);
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
		if(txtype==searchtype) {
			if ((txstate==TX_PROCEEDING)||(txstate==TX_INIT)) {
				*pTx=tx;
				return RC_OK;
			}
		}
	}

	/* if no match, return RC_ERROR */
	return RC_ERROR;
}

CCLAPI
RCODE 
sipDlgGetReqByBranchID(IN pSipDlg *const pdlg,OUT SipReq *pReq,IN const char* branchid)
{
	pSipDlg dlg;
	TxStruct tx;
	SipReq	req;
	char *bid;
	INT32 num,pos;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!pReq) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to req pointer\n" );
		return RC_ERROR;
	}

	if (!branchid) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetInviteTx] Invalid argument : Null pointer to branchid\n" );
		return RC_ERROR;
	}

	num=dxLstGetSize(dlg->txlist);
	for(pos=0;pos<num;pos++){
		tx=(TxStruct)dxLstPeek(dlg->txlist,pos);
		req=sipTxGetOriginalReq(tx);
		bid=GetReqViaBranch(req);
		if (strCmp(bid,branchid)==0) {
			*pReq=req;
			return RC_OK;
		}
	}

	/* if no match, return RC_ERROR */
	return RC_ERROR;
}

CCLAPI
RCODE 
sipDlgCheckComplete(IN pSipDlg *const pdlg,OUT BOOL *progress)
{
	pSipDlg dlg;
	TxStruct tx;
	TXTYPE txtype;
	TXSTATE txstate;
	INT32 num,pos;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgCheckComplete] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgCheckComplete] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!progress) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgCheckComplete] Invalid argument : Null pointer to progress\n" );
		return RC_ERROR;
	}

	*progress=TRUE;
	num=dxLstGetSize(dlg->txlist);
	for(pos=0;pos<num;pos++){
		tx=(TxStruct)dxLstPeek(dlg->txlist,pos);
		if(NULL==tx)
			continue;
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
		if ((txstate==TX_PROCEEDING)||(txstate==TX_INIT)||(txstate==TX_CALLING)) {
			*progress=FALSE;
			return RC_OK;
		}
	}

	return RC_OK;
}

/* get user data of this dialog */
CCLAPI 
RCODE 
sipDlgGetUserData(IN pSipDlg *const pdlg,OUT void** user)
{
	pSipDlg dlg;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetUserData] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetUserData] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!user) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetUserData] Invalid argument : Null pointer to user\n" );
		return RC_ERROR;
	}

	*user=dlg->userdata;
	return RC_OK;
}

/* set user data sequence of this dialog */
CCLAPI 
RCODE 
sipDlgSetUserData(IN pSipDlg *const pdlg,IN void *user)
{
	pSipDlg dlg;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetUserData] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetUserData] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	if( !user ) {
		DLGWARN("[sipDlgSetUserData] Invalid argument : Null pointer to user data\n" );
	}

	dlg->userdata=user;
	return RC_OK;
}

/* set allow-event header for 489 response */
CCLAPI 
RCODE 
sipDlgSetAllowEvent(IN const char *ae)
{
	RCODE rcode=RC_OK;
	char *tmp;

	if (!ae) {
		DLGWARN("[sipDlgSetUserData] Invalid argument : Null pointer to allow-events\n" );
		return RC_ERROR;
	}

	tmp=g_allowevent;
	g_allowevent=strDup(ae);
	if (NULL!=g_allowevent) {
		free(tmp);
		rcode=RC_OK;
	}else{
		/* strdup fail,so reload old value */
		g_allowevent=tmp;
		rcode=RC_ERROR;
	}

	return rcode;
}

/* check if PRACK is need */
CCLAPI 
RCODE 
sipDlgNeedPRAck(IN pSipDlg *const pdlg,OUT BOOL *needprack)
{
	pSipDlg dlg;
	
	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNeedPRAck] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNeedPRAck] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!needprack) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgNeedPRAck] Invalid argument : Null pointer to needprack\n" );
		return RC_ERROR;
	}
	
	*needprack=FALSE;
	if(dxLstGetSize(dlg->racklist)>0) *needprack=TRUE;

	return RC_OK;
}

CCLAPI 
BOOL sipDlgBeMatchTag(IN pSipDlg *const pdlg,IN const char* localtag,IN const char* remotetag)
{
	pSipDlg dlg;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgBeMatchTag] Invalid argument : Null pointer to dialog\n" );
		return FALSE;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgBeMatchTag] Invalid argument : Null pointer to dialog\n" );
		return FALSE;
	}

	if(BeMatchTag(dlg->localtag,localtag) && BeMatchTag(dlg->remotetag,remotetag))
		return TRUE;
	else
		return FALSE;

}

/************************************************************************/
/* internal function                                                    */
/************************************************************************/

RCODE sipDlgSetRRTrue(pSipDlg *const pdlg)
{
	pSipDlg dlg;

	/* check parameter */
	if ( !pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetRRTrue] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetRRTrue] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg->bRR=TRUE;
	return RC_OK;
}

RCODE 
sipDlgAddTx(pSipDlg *const pDlg,TxStruct tx)
{

	pSipDlg dlg;
	
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !tx ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddTx] Invalid argument : Null pointer to tx\n" );
		return RC_ERROR;
	}
	
	if(!dlg->txlist)
		dlg->txlist=dxLstNew(DX_LST_POINTER);
	
	return dxLstPutTail(dlg->txlist,(void*)tx);
}

RCODE 
sipDlgDelTx(pSipDlg *const pDlg,TxStruct tx)
{
	INT32 pos,num;
	pSipDlg dlg;
	SipReq req;
	SipMethodType method;
	SipSubState *substate=NULL;
	SipEvent	*event=NULL;

	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgDelTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgDelTx] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !tx ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgDelTx] Invalid argument : Null pointer to tx\n" );
		return RC_ERROR;
	}
	
	if(dlg->txlist){
		num=dxLstGetSize(dlg->txlist);
		for(pos=0;pos<num;pos++){
			if(tx==dxLstPeek(dlg->txlist,pos)){
				dxLstGetAt(dlg->txlist,pos);
				/* break; */
				DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgDelTx] Remove tx from dlg->txlist!\n" );
			}
		}
	}

	/*
	 *	remove event-list in dialog
	 */
	req=sipTxGetOriginalReq(tx);
	method=GetReqMethod(req);
	if(method==SIP_NOTIFY){
		/* register event to dialog */
		substate=(SipSubState *)sipReqGetHdr(req,Subscription_State);
		event=(SipEvent	*)sipReqGetHdr(req,Event);
		if(!substate || ! event)
			DLGWARN("[sipDlgDelTx] Bad notify!\n");
		
		if(!substate->state || !event->pack)
			DLGWARN("[sipDlgDelTx] Bad notify!\n");
		
		if(substate->state && event->pack){
			if(strICmp(substate->state,"terminated")==0){
				if(RC_OK!=sipDlgRemoveEvent(&dlg,GetReqEvent(req)))
					DLGWARN("[sipDlgDelTx] remove event is failed!\n");
			}
		}
	}
	if(dxLstGetSize(dlg->eventlist)==0)
		dlg->state=DLG_STATE_TERMINATED;

	/*sipTxFree(tx);*/
	sipTxSetUserData(tx,NULL);
	return RC_OK;
}

RCODE 
sipDlgGetTxSize(pSipDlg *const pDlg,int *listsize)
{

	pSipDlg dlg;
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetTxSize] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetTxSize] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !listsize ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgGetTxSize] Invalid argument : Null pointer to listsize\n" );
		return RC_ERROR;
	}
	
	if(dlg->txlist)
		*listsize=dxLstGetSize(dlg->txlist);
	else
		*listsize=0;
	
	return RC_OK;
}

RCODE 
sipDlgAddEvent(pSipDlg *const pDlg,const char *eventtype)
{

	pSipDlg dlg;
	UINT32 pos,num;
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddEvent] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddEvent] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if( !eventtype ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgAddEvent] Invalid argument : Null pointer to eventtype\n" );
		return RC_ERROR;
	}

	/* check eventlist */
	if(!dlg->eventlist)
		dlg->eventlist=dxLstNew(DX_LST_STRING);
	else{
		/* check if event is already in list */
		num=dxLstGetSize(dlg->eventlist);
		for(pos=0;pos<num;pos++){
			if(strICmp(eventtype,(const char*)dxLstPeek(dlg->eventlist,pos))==0)
				return RC_OK;
		}
	}
	/* add to eventlist */
	dxLstPutTail(dlg->eventlist,(void*)eventtype);
	return RC_OK;
}

RCODE 
sipDlgRemoveEvent(pSipDlg *const pDlg,const char *eventtype)
{
	pSipDlg dlg;
	UINT32 pos,num;
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgRemoveEvent] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgRemoveEvent] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if(!dlg->eventlist){
		DLGWARN("[sipDlgRemoveEvent] Invalid argument : Null pointer to eventlist\n" );
		return RC_ERROR;
	}
	if( !eventtype ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgRemoveEvent] Invalid argument : Null pointer to eventtype\n" );
		return RC_ERROR;
	}

	/* check eventlist */
	num=dxLstGetSize(dlg->eventlist);
	for(pos=0;pos<num;pos++){
		if(strICmp(eventtype,(char *)dxLstPeek(dlg->eventlist,pos))==0)
				free(dxLstGetAt(dlg->eventlist,pos));
	}
	
	return RC_OK;
}

void 
dlgFreeUserData(UserData *ud)
{
	FreeMemory(ud);
}

pSipDlg GetMatchDlg(SipMethodType method,const char *callid,const char* fromtag,const char *totag)
{
	pSipDlg dlg=NULL,tmpdlg;
	BOOL befromtag;
	BOOL betotag;

	Entry dlglist;
	
	dlglist=dxHashItemLst(DLGDBLst,callid);
	while ((tmpdlg=(pSipDlg)dxEntryGetValue(dlglist))) {
	
		befromtag=FALSE;
		betotag=FALSE;
		
		if(!strCmp(callid,tmpdlg->callid))
			;
		else{
			dlglist=dxEntryGetNext(dlglist);
			continue;
		}
		
		if(method!=SIP_NOTIFY){
			befromtag=BeMatchTag(fromtag,tmpdlg->remotetag);
		}else{
			if(tmpdlg->remotetag)
				befromtag=BeMatchTag(fromtag,tmpdlg->remotetag);
			else
				befromtag=TRUE;
		}

		if(tmpdlg->state>DLG_STATE_NULL){
			betotag=BeMatchTag(totag,tmpdlg->localtag);
		}

		if(betotag && befromtag){
			dlg=tmpdlg;
			break;
		}
		
		dlglist=dxEntryGetNext(dlglist);
	}	

	return dlg;
}

pSipDlg sipDlgMatchByTx(TxStruct txData)
{

	SipReq req=NULL;
	SipRsp rsp=NULL;
	
	char *callid=NULL,*fromtag=NULL,*totag=NULL;

	SipMethodType method;


	if( !txData )
		return NULL;

	req=sipTxGetOriginalReq(txData);
	rsp=sipTxGetLatestRsp(txData);
	
	if (!req || !rsp) return NULL;

	method=GetReqMethod(req);
	callid=(char *)sipReqGetHdr(req,Call_ID);
	fromtag=GetRspFromTag(rsp);
	totag=GetRspToTag(rsp);
	
	return GetMatchDlg(method,callid,fromtag,totag);

	/*
	dlglist=dxHashItemLst(DLGDBLst,callid);
	while ((tmpdlg=(pSipDlg)dxEntryGetValue(dlglist))) {
	
		befromtag=FALSE;
		betotag=FALSE;
		
		if(!strCmp(callid,tmpdlg->callid))
			;
		else{
			dlglist=dxEntryGetNext(dlglist);
			continue;
		}
		
		if(method!=SIP_NOTIFY){
			befromtag=BeMatchTag(fromtag,tmpdlg->remotetag);
		}else{
			if(tmpdlg->remotetag)
				befromtag=BeMatchTag(fromtag,tmpdlg->remotetag);
			else
				befromtag=TRUE;
		}

		if(tmpdlg->state>DLG_STATE_NULL){
			betotag=BeMatchTag(totag,tmpdlg->localtag);
		}

		if(betotag && befromtag){
			dlg=tmpdlg;
			break;
		}
		
		
		dlglist=dxEntryGetNext(dlglist);
	}

	return dlg;
	*/
}

/************************************************************************/
/* base on draft-ietf-sipping-dialogusage-01                                                                     */
/************************************************************************/
BOOL BeDestroyDlg(int statuscode)
{
	if((statuscode>=300)&&(statuscode<400))
		return TRUE;
	else if((statuscode>=400)&&(statuscode<500)){
		switch(statuscode) {
		case 404:
		case 408:
		case 410:
		case 416:
		case 481:
		case 482:
		case 483:
		case 484:
		case 485:
			return TRUE;
		default:
			return FALSE;
		}
	}else if((statuscode>=500)&&(statuscode<700)){
		switch(statuscode) {
		case 500:
		case 501:
		case 505:
		case 580:
		case 603:
		case 606:
			return FALSE;
		default:
			return TRUE;
		}	
	}else if(statuscode>=700)
		return TRUE;
	else
		return FALSE;
}

BOOL BeCreateDlg(SipMethodType method)
{
	switch(method) {
	case INVITE:
	case REFER:
	case SIP_SUBSCRIBE:
		return TRUE;
	default:
		return FALSE;
	}
}

RCODE addRAcklist(pSipDlg *const pDlg,const char* rack)
{
	pSipDlg dlg;
	
	/* check parameter */
	if ( ! pDlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[addRAcklist] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	dlg=*pDlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[addRAcklist] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	if((NULL==rack) || (strlen(rack)<1)){
		DlgPrintEx ( TRACE_LEVEL_API, "[addRAcklist] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	
	if(NULL==dlg->racklist){
		if(NULL==(dlg->racklist=dxLstNew(DX_LST_STRING))){
			DLGERR("create a list to store RAck is failed!");
			return RC_ERROR;
		}
	}

	/************************************************************************/
	/* todo:should check if it already exists.                              */
	/************************************************************************/

	return dxLstPutTail(dlg->racklist,(void *) rack);

}

RCODE 
sipDlgSetDB(pSipDlg *pdlg)
{
	pSipDlg dlg=NULL;
	
	/* check parameter */
	if ( !DLGDBLst) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetDB] run sipDlgInit first\n" );
		return RC_ERROR;
	}

	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetDB] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	dlg=*pdlg;
	if ( ! dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetDB] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}

	if(! dlg->callid) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetDB] Invalid argument : Null pointer to dialog's callid \n" );
		return RC_ERROR;
	}

	/* add to dialog list */
	dxHashAddEx(DLGDBLst,dlg->callid,(void*)dlg);
	DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgNew] dialog count:%d\n",dxHashSize(DLGDBLst));
	return RC_OK;
}


RCODE 
sipDlgRemoveFromDB(pSipDlg *pdlg)
{
	pSipDlg dlg;
	TxStruct tx;
	UserData* ud;
	INT32 pos,size;

	/* check parameter */
	if ( ! pdlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgFree] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	dlg=*pdlg;
	if( !dlg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgFree] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	if (!dlg->callid) {
		DLGWARN("callid is dlg is NULL!\n");
		return RC_ERROR;
	}

	/* remove from dialog db list */
	if (RC_OK!=dxHashDelEx(DLGDBLst,dlg->callid,(void*)dlg))
		DLGWARN("remove from dialog db list is failed!\n");
	
	if(dlg->txlist){
		DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgFree] remote txlist,count:%d\n",dxLstGetSize(dlg->txlist));
		/*dxLstFree(dlg->txlist,(void (*)(void *)) sipTxFree);*/
		size=dxLstGetSize(dlg->txlist);
		for (pos=0;pos<size;pos++){
			tx=(TxStruct)dxLstGetAt(dlg->txlist,0);
			if (tx) {
				ud=(UserData*)sipTxGetUserData(tx);
				if (ud) dlgFreeUserData(ud);
				sipTxFree(tx);
			}
		}
	}

	DlgPrintEx1 ( TRACE_LEVEL_API, "[sipDlgFree] total dialog count:%d\n",dxHashSize(DLGDBLst));
	return RC_OK;
}




