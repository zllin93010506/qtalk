/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * dlg_msg.c
 *
 * $Id: dlg_msg.c,v 1.2 2008/10/16 03:32:33 tyhuang Exp $
 */

#include "dlg_msg.h"
#include <sip/sip.h>

struct DlgMsgObj{
	pSipDlg		dlg;
	SipReq		reqmsg;
	SipRsp		rspmsg;
	void		*txhandle;

	/* RFC 3891 replace*/
	pSipDlg		replaceddlg;
};

CCLAPI 
RCODE 
sipDlgMsgGetDlg(IN pDlgMsg *const pMsg,OUT pSipDlg *pDlg)
{
	
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	
	if( !pDlg ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetDlg] Invalid argument : Null pointer to dlg\n" );
		return RC_ERROR;
	}
	*pDlg=msg->dlg;
	return RC_OK;
}

CCLAPI 
RCODE 
sipDlgMsgGetReq(IN pDlgMsg *const pMsg,OUT SipReq *pReq)
{
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	
	if( !pReq ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}
	*pReq=msg->reqmsg;
	return RC_OK;
}

CCLAPI 
RCODE 
sipDlgMsgGetRsp(IN pDlgMsg *const pMsg,OUT SipRsp *pRsp)
{
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	
	if( !pRsp ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReq] Invalid argument : Null pointer to request\n" );
		return RC_ERROR;
	}
	*pRsp=msg->rspmsg;
	return RC_OK;
}

CCLAPI 
RCODE 
sipDlgMsgGetHandle(IN pDlgMsg *const pMsg,OUT void **pHandle)
{
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetHandle] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetHandle] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	*pHandle=msg->txhandle;
	return RC_OK;

}

CCLAPI 
RCODE sipDlgMsgGetReplacedDlg(IN pDlgMsg *const pMsg,OUT pSipDlg* rdlg)
{
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReplacedDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReplacedDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}

	if( !rdlg ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgGetReplacedDlg] Invalid argument : Null pointer to dlg\n" );
		return RC_ERROR;
	}

	*rdlg=msg->replaceddlg;
	return RC_OK;
}

/* internal function */
/*create a new dialog layer message*/
RCODE 
sipDlgMsgNew(OUT pDlgMsg *pMsg,IN pSipDlg dlg,IN SipReq req,IN SipRsp rsp,IN void* tx)
{

	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgNew] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	if(tx==NULL){
		if( !dlg ) {
			DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgNew]  dlg is NULL\n" );
			return RC_ERROR;
		}
	}
	
	if( (!req) && (!rsp) && (!tx)) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgNew]  sip message is NULL\n" );
		return RC_ERROR;
	}
	msg=calloc(1,sizeof(DlgMsg));
	if( !msg ) {
		DLGWARN("[sipDlgMsgNew] Memory allocation error\n" );
		return RC_ERROR;
	}
	msg->dlg=dlg;
	/*
	if(req){
		msg->reqmsg=sipReqDup(req);
		msg->rspmsg=NULL;
	}else{
		msg->reqmsg=NULL;
		msg->rspmsg=sipRspDup(rsp);
	}*/
	if(req)
		msg->reqmsg=sipReqDup(req);
	
	if(rsp)
		msg->rspmsg=sipRspDup(rsp);
	
	msg->txhandle=tx;
	*pMsg=msg;
	return RC_OK;
}


RCODE 
sipDlgMsgFree(IN OUT pDlgMsg* pMsg)
{

	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgNew] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgMsgNew] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	
	if(msg->reqmsg)
		sipReqFree(msg->reqmsg);
	if(msg->rspmsg)
		sipRspFree(msg->rspmsg);
	memset(msg,0,sizeof(DlgMsg));
	free(msg);
	*pMsg=NULL;
	return RC_OK;
}

RCODE 
sipDlgMsgSetReplacedDlg(IN pDlgMsg *const pMsg,IN pSipDlg rdlg)
{
	pDlgMsg msg=NULL;
	/* check parameter */
	if ( ! pMsg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetReplacedDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}
	msg=*pMsg;
	if( !msg ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgSetReplacedDlg] Invalid argument : Null pointer to dlgmsg\n" );
		return RC_ERROR;
	}

	msg->replaceddlg=rdlg;
	return RC_OK;

}
