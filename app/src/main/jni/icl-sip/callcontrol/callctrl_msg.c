/* Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * callctrl_msg.c
 *
 * $Id: callctrl_msg.c,v 1.2 2008/10/16 03:32:41 tyhuang Exp $
 */
/** \file callctrl_msg.c
 *  \brief Define ccMsg struct access functions
 *  \author Huang,Teng Yi
 *  \remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
 */

#include <sip/sip.h>
#include "callctrl_msg.h"
#include "callctrl_cm.h"
#include "callctrl_call.h"

struct ccMsgObj{
	pCCCall		call;
	SipReq		reqmsg;
	SipRsp		rspmsg;

	/* 
	 * store current state for delay process event of upper layer or
	 * multi-thread process
	 */
	ccCallState	cs;
	pSipDlg		dlghandle;
	CallEvtType	event;

	pCCCall		replacedcall;
};

/* get call object from callctrl Msg */
CCLAPI 
RCODE ccMsgGetCALL(IN pCCMsg *const pMsg,OUT pCCCall *pcall)
{

	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if (!pcall) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	*pcall=msg->call;

	return RC_OK;
}

CCLAPI 
RCODE ccMsgGetCALLState(IN pCCMsg* const pMsg,OUT ccCallState *cstate)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetReq] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetReq] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	*cstate=msg->cs;
	return RC_OK;
}

/* get request  from callctrl Msg */
CCLAPI 
RCODE ccMsgGetReq(IN pCCMsg *const pMsg,OUT SipReq *pReq)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetReq] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetReq] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if (!pReq) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetReq] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}
	*pReq=msg->reqmsg;

	return RC_OK;
}

/* get response  from callctrl Msg */
CCLAPI 
RCODE ccMsgGetRsp(IN pCCMsg *const pMsg,OUT SipRsp *pRsp)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetRsp] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetRsp] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if (!pRsp) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetRsp] Invalid argument : Null pointer to response\n" );
		return RC_ERROR;
	}
	*pRsp=msg->rspmsg;

	return RC_OK;
}

/* get response  from callctrl Msg */
CCLAPI 
RCODE ccMsgGetCallLeg(IN pCCMsg* const pMsg,OUT pSipDlg *pDlg)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetDlg] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetDlg] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if (!pDlg) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetDlg] Invalid argument : Null pointer to response\n" );
		return RC_ERROR;
	}
	*pDlg=msg->dlghandle;

	return RC_OK;
}

CCLAPI 
RCODE ccMsgGetEvent(IN pCCMsg* const pMsg,OUT CallEvtType *pEvt)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetEvent] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetEvent] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	*pEvt=msg->event;
	return RC_OK;
}

CCLAPI 
RCODE ccMsgDup(IN pCCMsg*const source,OUT pCCMsg* dest)
{
	pCCMsg msg=NULL,newmsg;
	/* check parameter */
	if ( ! source ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgDup] Invalid argument : Null pointer to source\n" );
		return RC_ERROR;
	}

	if ( ! dest ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgDup] Invalid argument : Null pointer to destination\n" );
		return RC_ERROR;
	}

	msg=*source;

	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgDup] Invalid argument : Null pointer to source\n" );
		return RC_ERROR;
	}
	
	newmsg=calloc(1,sizeof(CCMsg));
	if( !msg ) {
		CCWARN("[ccMsgDup] Memory allocation error\n" );
		return RC_ERROR;
	}
	newmsg->call=msg->call;
	if(msg->reqmsg)
		newmsg->reqmsg=sipReqDup(msg->reqmsg);
	else
		newmsg->reqmsg=NULL;
	
	if(msg->rspmsg)
		newmsg->rspmsg=sipRspDup(msg->rspmsg);
	else
		newmsg->rspmsg=NULL;

	newmsg->cs=msg->cs;
	newmsg->event=msg->event;
	newmsg->dlghandle=msg->dlghandle;

	newmsg->replacedcall=msg->replacedcall;
	
	*dest=newmsg;
	
	return RC_OK;
}

/* ccMsgFree */
CCLAPI
RCODE ccMsgFree(IN OUT pCCMsg *pMsg)
{

	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgFree] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgFree] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	
	if(msg->reqmsg)
		sipReqFree(msg->reqmsg);
	if(msg->rspmsg)
		sipRspFree(msg->rspmsg);
	memset(msg,0,sizeof(CCMsg));
	free(msg);
	*pMsg=NULL;
	return RC_OK;
}


CCLAPI RCODE ccMsgGetReplacedCall(IN pCCMsg* const pMsg,OUT pCCCall *rpcall )
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}

	if (!rpcall) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgGetCALL] Invalid argument : Null pointer to call\n" );
		return RC_ERROR;
	}
	*rpcall=msg->replacedcall;

	return RC_OK;	
}

/* internal function */
/*create a new callctrl layer message*/
/** \fn RCODE ccMsgNew(IN OUT pCCMsg *pMsg,IN pCCCall call,IN SipReq req,IN SipRsp rsp,IN pSipDlg dlg)
 *	\brief Function to new a callctrl message object;for internal use
 */
RCODE ccMsgNew(IN pCCMsg *pMsg,IN pCCCall call,IN SipReq req,IN SipRsp rsp,IN pSipDlg dlg)
{
	pCCMsg msg=NULL;
	ccCallState cstate=CALL_STATE_NULL;
	
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgNew] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	
	if( !call ) {
			CCPrintEx ( TRACE_LEVEL_API, "[ccMsgNew]  call is NULL\n" );
			return RC_ERROR;
	}
	
	if( (!req) && (!rsp) ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgNew]  sip message is NULL\n" );
		return RC_ERROR;
	}

	msg=calloc(1,sizeof(CCMsg));
	if( !msg ) {
		CCWARN("[ccMsgNew] Memory allocation error\n" );
		return RC_ERROR;
	}
	msg->call=call;
	if(req)
		msg->reqmsg=sipReqDup(req);
	else
		msg->reqmsg=NULL;
	if(rsp)
		msg->rspmsg=sipRspDup(rsp);
	else
		msg->rspmsg=NULL;

	if(RC_OK==ccCallGetCallState(&call,&cstate))
		msg->cs=cstate;
	else
		CCWARN("[ccMsgNew] fail to store call state\n" );

	msg->dlghandle=dlg;
	msg->event=NULL_EVENT;
	*pMsg=msg;
	return RC_OK;
}

RCODE ccMsgSetEvent(IN pCCMsg *pMsg ,IN CallEvtType evt)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgSetEvent] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgSetEvent] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg->event=evt;
	return RC_OK;
}

RCODE ccMsgSetReplacedCall(IN pCCMsg *pMsg,IN pCCCall rcall)
{
	pCCMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgSetReplacedCall] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		CCPrintEx ( TRACE_LEVEL_API, "[ccMsgSetReplacedCall] Invalid argument : Null pointer to msg\n" );
		return RC_ERROR;
	}
	msg->replacedcall=rcall;
	return RC_OK;
}





