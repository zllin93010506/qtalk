/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * dlg_sipmsg.c
 *
 * $Id: dlg_sipmsg.c,v 1.11 2009/03/04 05:11:52 tyhuang Exp $
 */

#include <stdio.h>

#include <common/cm_def.h>
#include <adt/dx_str.h>
#include <sip/sip.h>

#include "dlg_sipmsg.h"

#ifdef _WIN32_WCE /*WinCE didn't support time.h*/
#elif defined(_WIN32)
#include <time.h>
#elif defined(UNIX)
#include <time.h>
#include <sys/time.h>
#endif

char * 
sipDlgReqChk(IN SipReq req)
{
	char *callid=NULL;

	SipReplaces *replaces;
	
	/* check parameter */
	if( !req ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to dialog\n" );
		/* return RC_ERROR; */
		return "bad request - request parsing error";
	}
	/* check Request URI */
	if(GetReqUrl(req)==NULL){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to request uri \n" );
		/* return RC_ERROR; */
		return "bad request - request url is null";
	}

	/* check method type */
	if(GetReqMethod(req)==UNKNOWN_METHOD){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : UNKNOWN Method \n" );
		/* return RC_ERROR; */
		return "bad request - method not allow";
	}

	/* check call id */
	callid=(char*)sipReqGetHdr(req,Call_ID);
	if( !callid ){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to call-id \n" );
		/* return RC_ERROR; */
		return "bad request - callid is null";
	}
	
	/* check cseq */
	if(sipReqGetHdr(req,CSeq)==NULL){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to cseq \n" );
		/* return RC_ERROR; */
		return "bad request - no cseq";
	}

	/* check from url */
	if(GetReqFromUrl(req)==NULL){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to from url \n" );
		/* return RC_ERROR; */
		return "bad request - no from url";
	}
	
	/* check To url */
	if(GetReqToUrl(req)==NULL){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to to url \n" );
		/* return RC_ERROR; */
		return "bad request - no to url";
	}

	/* check max-forwards */
	if(*(int*)sipReqGetHdr(req,Max_Forwards)<0){
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : no max-forwards \n" );
		/* return RC_ERROR; */
		return "bad request - no maxforwds";
	}

	/* check Via branch id ,RFC3261 */
	if (GetReqViaBranch(req)==NULL){ 
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] Invalid argument : Null pointer to via branch \n" );
		/* return RC_ERROR; */
		return "bad request - no branch id (RFC 3261)";
	}

	if(NULL!=(replaces=(SipReplaces *)sipReqGetHdr(req,Replaces))){

		if(INVITE!=GetReqMethod(req)){
				DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] RFC 3891 : a Replaces header field is present in a request other than INVITE!\n" );
				/* return RC_ERROR; */
				return "bad request - method is no invite (RFC 3891)";
		}
		
		/* method is INVITE , check replace */
		if(replaces->next!=NULL){
			DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgReqChk] RFC 3891 : more than One Replaces header field is present in a request !\n" );
			/* return RC_ERROR; */
			return "bad request - more than one Replaces header (RFC 3891)";
		}
	}

	/* return RC_OK; */
	return NULL;
}

RCODE 
sipDlgRspChk(IN SipRsp rsp)
{
	char *callid=NULL;
	/* check parameter */
	if( !rsp ) {
		DlgPrintEx ( TRACE_LEVEL_API, "[sipDlgRspChk] Invalid argument : Null pointer to dialog\n" );
		return RC_ERROR;
	}
	callid=(char*)sipRspGetHdr(rsp,Call_ID);
	if( !callid )
		return RC_ERROR;
	
	if(sipRspGetHdr(rsp,CSeq)==NULL)
		return RC_ERROR;

	/* check from url */
	if(GetRspFromUrl(rsp)==NULL)
		return RC_ERROR;
	
	if(GetRspToUrl(rsp)==NULL)
		return RC_ERROR;

	return RC_OK;
}

/*Get From and To header url */
char*
GetReqFromUrl(SipReq _req)
{
	SipFrom *pFrom=NULL;
	
	if(NULL == _req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(_req,From);
	if(NULL == pFrom)
		return NULL;
	
	if(NULL == pFrom->address)
		return NULL;

	return pFrom->address->addr;
}

char*	
GetReqToUrl(SipReq _req)
{
	SipTo *pTo=NULL;

	if(NULL == _req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(_req,To);
	if(NULL == pTo)
		return NULL;
	
	if(NULL == pTo->address)
		return NULL;
	
	return pTo->address->addr;
}

/*Get From and To header tag */

char*	
GetReqFromTag(SipReq _req)
{
	SipFrom *pFrom=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(_req,From);
	if(NULL == pFrom)
		return NULL;

	pParam=pFrom->paramList;
	for(idx=0;(idx<pFrom->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return NULL;
}

char*	
GetReqToTag(SipReq _req)
{
	SipTo *pTo=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(_req,To);
	if(NULL == pTo)
		return NULL;

	pParam=pTo->paramList;
	for(idx=0;(idx<pTo->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return NULL;
}

char*	
GetReqContact(SipReq req)
{
	SipContact *pContact;
	if( !req )
		return NULL;
	
	pContact=(SipContact*)sipReqGetHdr(req,Contact);
	if(pContact){
		if(pContact->sipContactList)
			if(pContact->sipContactList->address)
				return pContact->sipContactList->address->addr;
	}
	return NULL;
}

char*	
GetReqUrl(IN SipReq req)
{
	SipReqLine *reqline;
	
	if(!req) return NULL;

	reqline=sipReqGetReqLine(req);
	if (!reqline) return NULL;
	
	return reqline->ptrRequestURI;
}

char*	
GetReqFromDisplayname(IN SipReq req)
{
	SipFrom *pFrom=NULL;
	
	if(NULL == req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(req,From);
	if(NULL == pFrom)
		return NULL;
	
	if(NULL == pFrom->address)
		return NULL;

	return pFrom->address->display_name;	
}

char*	
GetReqToDisplayname(IN SipReq req)
{
	SipTo *pTo=NULL;

	if(NULL == req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(req,To);
	if(NULL == pTo)
		return NULL;

	if(NULL == pTo->address)
		return NULL;

	return pTo->address->display_name;
}

char*
GetReqReferToUrl(IN SipReq req)
{
	SipReferTo *referto=NULL;

	if(!req)
		return NULL;

	referto=(SipReferTo*)sipReqGetHdr(req,Refer_To);

	if(referto && referto->address)
		return referto->address->addr;
	else
		return NULL;
}

char*	
GetRspFromUrl(SipRsp rsp)
{
	SipFrom *pFrom=NULL;
	
	if(NULL == rsp)
		return NULL;

	pFrom=(SipFrom*)sipRspGetHdr(rsp,From);
	if(NULL == pFrom)
		return NULL;
	
	if(NULL == pFrom->address)
		return NULL;

	return pFrom->address->addr;
}

char*	
GetRspToUrl(SipRsp rsp)
{
	SipTo *pTo=NULL;

	if(NULL == rsp)
		return NULL;

	pTo=(SipTo*)sipRspGetHdr(rsp,To);
	if(NULL == pTo)
		return NULL;
	
	if(NULL == pTo->address)
		return NULL;
	else
		return pTo->address->addr;
}

char*	
GetRspToDisplayname(IN SipRsp rsp)
{
	SipTo *pTo=NULL;

	if(NULL == rsp)
		return NULL;

	pTo=(SipTo*)sipRspGetHdr(rsp,To);
	if(NULL == pTo)
		return NULL;
	
	if(NULL == pTo->address)
		return NULL;
	else
		return pTo->address->display_name;	
}

RCODE	
AddRspToTag(SipRsp rsp,const char* tag)
{
	SipTo *pTo;
	SipParam *newparam;

	if( !rsp )
		return RC_ERROR;

	pTo=(SipTo*)sipRspGetHdr(rsp,To);
	if(pTo){
		newparam=(SipParam*)calloc(1,sizeof(SipParam));
		if( !newparam ){
			DlgPrintEx ( TRACE_LEVEL_API, "[AddRspToTag] Invalid argument : Memory allocation error\n" );
			return RC_ERROR;
		}
		newparam->name=strDup("tag");
		newparam->value=strDup(tag);
		newparam->next=pTo->paramList;
		pTo->paramList=newparam;
		pTo->numParam++;
	}
	return RC_OK;
}

char*	
GetRspToTag(SipRsp _rsp)
{
	SipTo *pTo=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _rsp)
		return NULL;

	pTo=(SipTo*)sipRspGetHdr(_rsp,To);
	if(NULL == pTo)
		return NULL;

	pParam=pTo->paramList;
	for(idx=0;(idx<pTo->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return NULL;
}

char*	
GetRspFromTag(SipRsp _rsp)
{
	SipFrom *pFrom=NULL;
	INT32 idx;
	SipParam *pParam=NULL;
	
	if(NULL == _rsp)
		return NULL;

	pFrom=(SipFrom*)sipRspGetHdr(_rsp,From);
	if(NULL == pFrom)
		return NULL;

	pParam=pFrom->paramList;
	for(idx=0;(idx<pFrom->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return NULL;
}

INT32 
GetRspStatusCode(SipRsp _rsp)
{
	SipRspStatusLine *pLine;
	INT32 code=-1;

	if(_rsp!=NULL){
		pLine=sipRspGetStatusLine(_rsp);
		if(pLine != NULL){
			code=a2i(pLine->ptrStatusCode);
		}
	}
	return code;
}

char*	
GetRspContact(SipRsp rsp)
{
	SipContact *pContact;
	if( !rsp )
		return NULL;
	
	pContact=(SipContact*)sipRspGetHdr(rsp,Contact);
	if(pContact){
		if(pContact->sipContactList)
			if(pContact->sipContactList->address)
				return pContact->sipContactList->address->addr;
	}
	return NULL;
}

INT32 
GetRspContactExpires(IN SipRsp rsp)
{
	SipContact *pContact;
	INT32 expires=-1;
	
	if( !rsp )
		return expires;
	
	expires=GetRspExpires(rsp);

	pContact=(SipContact*)sipRspGetHdr(rsp,Contact);
	if(pContact){
		if(pContact->sipContactList)
			if(pContact->sipContactList->address)
				expires=pContact->sipContactList->expireSecond;
	}

	return expires;
}

INT32 GetReqExpires(IN SipReq req)
{
	SipExpires *pExpirehdr;
	INT32 expires=-1;
	
	if(!req)
		return expires;
	
	pExpirehdr=(SipExpires*)sipReqGetHdr(req,Expires);
	if(pExpirehdr)
		expires=pExpirehdr->expireSecond;
	
	return expires;
}

INT32 
GetRspExpires(IN SipRsp rsp)
{
	SipExpires *pExpirehdr;
	INT32 expires=-1;
	
	if(!rsp)
		return expires;

	pExpirehdr=(SipExpires*)sipRspGetHdr(rsp,Expires);
	if(pExpirehdr)
		expires=pExpirehdr->expireSecond;
	
	return expires;
}

char* 
NewRealmFromRsp(IN SipRsp rsp)
{
	int statuscode;

	SipAuthnStruct tmpAuth;
	RCODE ret;

	if(!rsp)
		return NULL;

	statuscode=GetRspStatusCode(rsp);
	if((statuscode != 401)&&(statuscode != 407))
		return NULL;
	
	tmpAuth.iType=SIP_DIGEST_CHALLENGE;
	switch(statuscode){
	case 401:
		ret=sipRspGetWWWAuthn(rsp,&tmpAuth);
		break;
	case 407:
		ret=sipRspGetPxyAuthn(rsp,&tmpAuth);
		break;
	}

	if(ret != RC_ERROR){
		return strDup(tmpAuth.auth_.digest_.realm_);
	}else
		return NULL;
}

SipMethodType 
GetReqMethod(SipReq req)
{
	SipMethodType method=UNKNOWN_METHOD;
	SipReqLine *pLine=NULL;

	if(req != NULL){
		pLine=sipReqGetReqLine(req);
		if(pLine != NULL){
			method=pLine->iMethod;
		}
	}
	return method;
}

SipMethodType 
GetRspMethod(SipRsp _rsp)
{
	SipMethodType method=UNKNOWN_METHOD;
	SipCSeq	*pCSeq=NULL;

	if(_rsp != NULL){
		pCSeq=sipRspGetHdr(_rsp,CSeq);
		if(pCSeq != NULL){
			method=pCSeq->Method;
		}
	}
	return method;
}

char* 
GetRspReason(SipRsp _rsp)
{
	SipRspStatusLine *rspstatusline;

	if(!_rsp)
		return NULL;
	rspstatusline=sipRspGetStatusLine(_rsp);
	if(!rspstatusline)
		return NULL;
	return rspstatusline->ptrReason;
}

char* 
GetReqEvent(SipReq req)
{

	SipEvent *event=NULL;
	static char buf[256];
	INT32 pos;

	SipStr *_str;
	SipParam *_param;

	if(!req)
		return NULL;

	event=sipReqGetHdr(req,Event);

	if(!event)
		return NULL;

	if(!event->pack)
		return NULL;

	/* get eventpack */
	pos=snprintf(buf, sizeof(buf),"%s",event->pack);

	/* get tmplate of eventpack */
	_str=event->tmplate;
	while(_str!=NULL){
		pos+=snprintf(buf+pos, sizeof(buf+pos),".%s",_str->str);
		_str=_str->next;
	}
	/* check id parameter if exist must,add it*/
	_param=event->paramList;
	while(_param!=NULL){
		
		if(strCmp(_param->name,"id")==0){
			pos+=snprintf(buf+pos, sizeof(buf+pos),";%s",_param->name);
			if(_param->value)
				pos+=snprintf(buf+pos, sizeof(buf+pos),"=%s",_param->value);
			break;
		}
	
		_param=_param->next;
	}

	return buf;

}

int GetReqEventID(IN SipReq req)
{

	SipEvent *event=NULL;
	SipParam *_param;
	
	if(!req)
		return -1;

	event=sipReqGetHdr(req,Event);

	if(NULL==event)
		return -1;

	_param=event->paramList;
	while(_param!=NULL){
		
		if(strCmp(_param->name,"id")==0){
			if(_param->value)
				return a2i(_param->value);
			else
				return -1;
		}
		_param=_param->next;
	}

	return -1;
}

char* 
GetReqSubState(IN SipReq req)
{
	SipSubState *substate;

	if(!req) return NULL;
	
	substate=sipReqGetHdr(req,Subscription_State);

	if(substate)
		return substate->state;
		
	return NULL;
}

char* 
GetReqViaBranch(SipReq req)
{
	SipVia *via;
	SipViaParm *tmpViaPara;

	if(!req)
		return NULL;
	
	via=(SipVia*)sipReqGetHdr(req,Via);
	if(!via)
		return NULL;
	
	tmpViaPara=via->ParmList;
	if(!tmpViaPara)
		return NULL;

	return tmpViaPara->branch;
}

char* 
GetRspViaBranch(SipRsp rsp)
{
	SipVia *via;
	SipViaParm *tmpViaPara;

	if(!rsp)
		return NULL;
	
	via=(SipVia*)sipRspGetHdr(rsp,Via);
	if(!via)
		return NULL;
	
	tmpViaPara=via->ParmList;
	if(!tmpViaPara)
		return NULL;

	return tmpViaPara->branch;
}

char* 
GetRspViaSentBy(SipRsp rsp)
{
	SipVia *via;
	SipViaParm *tmpViaPara;

	if(!rsp)
		return NULL;
	
	via=(SipVia*)sipRspGetHdr(rsp,Via);
	if(!via)
		return NULL;
	
	tmpViaPara=via->ParmList;
	if(!tmpViaPara)
		return NULL;

	return tmpViaPara->sentby;
}

char* PrintRequestline(IN SipReq req)
{
	static char buf[512];
	SipReqLine* reqline;

	char* outtext;
	DxStr tmpbuf=NULL;

	memset(buf,0,sizeof(buf));
	if (!req) return buf;

	reqline=sipReqGetReqLine(req);
	if(!reqline) return buf;

	tmpbuf=dxStrNew();
	if(NULL==tmpbuf)
		return buf;
	
	dxStrCat(tmpbuf,sipMethodTypeToName(reqline->iMethod));
	dxStrCat(tmpbuf," ");
	dxStrCat(tmpbuf,reqline->ptrRequestURI);
	dxStrCat(tmpbuf," ");
	dxStrCat(tmpbuf,reqline->ptrSipVersion);

	outtext=dxStrAsCStr(tmpbuf);
	if(outtext && strlen(outtext)<512)
		snprintf(buf, sizeof(buf),"%s",outtext);

	dxStrFree(tmpbuf);
	/*
	if (reqline) 
		sprintf(buf,"%s %s %s",sipMethodTypeToName(reqline->iMethod),reqline->ptrRequestURI,reqline->ptrSipVersion);
	*/
	return buf;
}

char* PrintStatusline(IN SipRsp rsp)
{
	static char buf[512];
	SipRspStatusLine *rspline;

	char* outtext;
	DxStr tmpbuf=NULL;

	memset(buf,0,sizeof(buf));
	if (!rsp) return buf;

	rspline=sipRspGetStatusLine(rsp);
	if(!rspline) return buf;

	tmpbuf=dxStrNew();
	if(NULL==tmpbuf)
		return buf;

	dxStrCat(tmpbuf,rspline->ptrVersion);
	dxStrCat(tmpbuf," ");
	dxStrCat(tmpbuf,rspline->ptrStatusCode);
		dxStrCat(tmpbuf," ");
	dxStrCat(tmpbuf,rspline->ptrReason);

	outtext=dxStrAsCStr(tmpbuf);
	if(outtext && strlen(outtext)<512)
		snprintf(buf, sizeof(buf),"%s",outtext);

	dxStrFree(tmpbuf);
	/*
	if (rspline) 
		sprintf(buf,"%s %s %s",rspline->ptrVersion,rspline->ptrStatusCode,rspline->ptrReason);
	*/

	return buf;
}

SipRoute* 
RecordRouteCovertToRoute(IN SipRecordRoute* _pRecord)
{
	SipRoute *pRoute;
	SipRecAddr *tmpAddr,*newAddr,*preAddr;
	INT32 i=0;

	/* Route Header Copy From Record-Route header and reverse, in the same call-leg */
	if(_pRecord!= NULL){ /*need to copy reverse into Route*/
		pRoute=(SipRoute*) calloc(1,sizeof(SipRoute));
		tmpAddr=_pRecord->sipNameAddrList;
		for(i=0;i<_pRecord->numNameAddrs;i++) {
			if(tmpAddr == NULL) 
				break;
			newAddr=(SipRecAddr*)calloc(1,sizeof(SipRecAddr));
			newAddr->next=NULL;
			if(tmpAddr->addr != NULL)
				newAddr->addr=strDup(tmpAddr->addr);
			else	
				newAddr->addr=NULL;
			if(tmpAddr->display_name != NULL)
				newAddr->display_name=strDup(tmpAddr->display_name);
			else
				newAddr->display_name=NULL;
			if(tmpAddr->sipParamList){
				newAddr->sipParamList=sipParameterDup(tmpAddr->sipParamList,tmpAddr->numParams);
				newAddr->numParams=tmpAddr->numParams;
			}
			(i==0)? (preAddr=newAddr):(newAddr->next=preAddr);
			tmpAddr=tmpAddr->next;
			preAddr=newAddr;
		}
		if(i>0){
			pRoute->numNameAddrs=i;
			pRoute->sipNameAddrList=preAddr;
		}
		return pRoute;
	}else
		return NULL;
}

RCODE 
SipRouteFree(IN SipRoute* _route)
{
	RCODE rCode=RC_OK;

	SipRecAddr *tmpAddr;

	if(_route!=NULL){
		tmpAddr=_route->sipNameAddrList;
		if(tmpAddr!=NULL){
			sipRecAddrFree(tmpAddr);
			tmpAddr=NULL;
		}
		free(_route);
		_route=NULL;
	}else
		rCode=RC_ERROR;

	return rCode;
}

DxLst 
GetReqRecRoute(SipReq req)
{
	SipRecordRoute *pRecordRoute=NULL;
	SipRecAddr *_addr;
	DxLst routeset=NULL;
	INT32 i,size;
	
	pRecordRoute=(SipRecordRoute*)sipReqGetHdr(req,Record_Route);

	if(pRecordRoute != NULL){
			size=pRecordRoute->numNameAddrs;
			routeset=dxLstNew(DX_LST_STRING);
			_addr = pRecordRoute->sipNameAddrList;
			for(i=0;i<size;i++){
				dxLstPutTail(routeset,_addr->addr);
				_addr = _addr->next;
				if(_addr==NULL) break;
			}
	}
	
	return routeset;
}

DxLst 
GetRspRecRoute(SipRsp rsp)
{
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipRecAddr *_addr;
	DxLst routeset=NULL;
	INT32 i,size;
	
	pRecordRoute=(SipRecordRoute*)sipRspGetHdr(rsp,Record_Route);
	if(pRecordRoute != NULL)
		pRoute=RecordRouteCovertToRoute(pRecordRoute);

	if(pRoute != NULL){
			size=pRoute->numNameAddrs;
			routeset=dxLstNew(DX_LST_STRING);
			_addr = pRoute->sipNameAddrList;
			for(i=0;i<size;i++){
				dxLstPutTail(routeset,_addr->addr);
				_addr = _addr->next;
				if(_addr==NULL) break;
			}
			SipRouteFree(pRoute);
	}
	
	return routeset;
}

unsigned int GetReqCSeq(IN SipReq req)
{
	if(NULL==req){
		return 0;
	}else{
		SipCSeq *cseq=(SipCSeq *)sipReqGetHdr(req,CSeq);

		if(NULL==cseq){
			return 0;
		}else
			return cseq->seq;
	}

}

/* be sip ,sips or tel */
BOOL beScheme(const char* addr)
{
	char *colon;
	char *newaddr;

	if(!addr)
		return FALSE;
	else
		newaddr=strDup(addr);

	if(!newaddr)
		return FALSE;

	colon=strchr(newaddr,':');
	
	if(NULL==colon){
		free(newaddr);
		return FALSE;
	}

	*colon='\0';

	if(strstr(newaddr,"sip") || strstr(newaddr,"sips") || strstr(newaddr,"tel")){
		free(newaddr);
		return TRUE;
	}else{
		free(newaddr);
		return FALSE;
	}
}

char* NewNOPortURL(const char* url)
{
	char *newurl=NULL;
	char *right=NULL; /* for searching ']' */
	char *colon=NULL; /* for searching ':' */
	char *start=NULL;

	if(NULL==url)
		return NULL;

	newurl=strDup(url);
	if(!newurl)
		return NULL;

	if(beScheme(newurl))
		start=strchr(newurl,':')+1;
	else
		start=newurl;

	if((right=strchr(start,']'))!=NULL){
		colon=strchr(right,':');
	}else
		colon=strchr(start,':');

	if(colon){
		char* nextcolon=strchr(colon+1,':');
		if(nextcolon)
			colon=nextcolon;
		*colon='\0';
	}

	return newurl;
}

char* 
taggen(void)
{
	/*static char buf[128]={'\0'};*/
	char *buf=NULL;
	static UINT32 count;
	UINT32 ltime=GetRand();
		
	buf=(char *)calloc(32,sizeof (char));
	snprintf(buf, sizeof(buf),"%s-%05d%04d",OWNER,ltime,++count);

	return buf;
}

UINT32 GetRand()
{
	UINT32 ltime;
	
#ifdef _WIN32_WCE
	ltime=rand();
#elif defined (_WIN32)	
	ltime=clock();
#elif defined(UNIX)
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(unsigned long) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
	ltime=rand();
#endif
	return ltime;
}

BOOL BeRequireOption(IN SipReq req,IN const char *tag)
{
	SipRequire *require=NULL;
	SipStr *optionlist=NULL;
	
	int i;
	
	if(!req) return FALSE;
	
	require = (SipRequire *)sipReqGetHdr(req,Require);
	if(!require || !require->sipOptionTagList) return FALSE;
	
	/* check if tag is in option list */
	optionlist=require->sipOptionTagList;
	for(i=0;i<require->numTags;i++){
		if( strICmp(optionlist->str,tag)==0)
			return TRUE;
		else
			optionlist=optionlist->next;
		
		if(NULL==optionlist) break;
	}
	
	return FALSE;
}

BOOL BeRequire100rel(IN SipReq req)
{
	return BeRequireOption(req,"100rel");
}

BOOL BeSupportOption(IN SipReq req,IN const char *tag)
{
	SipSupported *support=NULL;
	SipStr *optionlist=NULL;
	
	int i;
	
	if((NULL==req) || (NULL==tag)) return FALSE;
	
	support = (SipRequire *)sipReqGetHdr(req,Supported);
	if(!support || !support->sipOptionTagList) return FALSE;
	
	/* check if tag is in option list */
	optionlist=support->sipOptionTagList;
	for(i=0;i<support->numTags;i++){
		if( strICmp(optionlist->str,tag)==0)
			return TRUE;
		else
			optionlist=optionlist->next;
		
		if(NULL==optionlist) break;
	}
	
	return FALSE;
}

BOOL BeSupport100rel(IN SipReq req)
{
	return BeSupportOption(req,"100rel");
}

BOOL Be100rel(SipRsp rsp)
{
	SipRequire *require=NULL;
	SipStr *optionlist=NULL;
	
	int i;
	
	if(!rsp) return FALSE;
	
	require = (SipRequire *)sipRspGetHdr(rsp,Require);
	if(!require || !require->sipOptionTagList) return FALSE;
	
	/* check if "100rel" is in option list */
	optionlist=require->sipOptionTagList;
	for(i=0;i<require->numTags;i++){
		if( strICmp(optionlist->str,"100rel")==0)
			return TRUE;
		else
			optionlist=optionlist->next;
		
		if(NULL==optionlist) break;
	}
	
	return FALSE;
}

BOOL CheckOptionTag(IN SipReq req)
{
	SipRequire *rq;
	SipPxyRequire *prq;

	if(NULL==req) return TRUE;

	rq=(SipRequire *)sipReqGetHdr(req,Require);
	if(rq && rq->sipOptionTagList && rq->sipOptionTagList->str){
		if(strstr(SUPPORT_EXTEND,rq->sipOptionTagList->str)==NULL)
			return FALSE;
	}
	
	prq=(SipRequire *)sipReqGetHdr(req,Proxy_Require);
	if(prq && prq->sipOptionTagList && prq->sipOptionTagList->str){
		if(strstr(SUPPORT_EXTEND,prq->sipOptionTagList->str)==NULL)
			return FALSE;
	}
	
	return TRUE;
}


RCODE AddRspRSeq(IN SipRsp rsp,IN UINT32 rseq)
{
	char buffer[DLGBUFFERSIZE];

	if(!rsp) return FALSE;


	memset(buffer,0, DLGBUFFERSIZE*sizeof(char));
	snprintf(buffer, DLGBUFFERSIZE*sizeof(char),"RSeq:%d",rseq);
	return sipRspAddHdrFromTxt(rsp,RSeq,buffer);
	
}

BOOL BeMatchTag(const char* s1,const char* s2)
{
	if((NULL==s1)&&(NULL==s2))
		return TRUE;
	if((NULL==s1)||(NULL==s2))
		return FALSE;

	if(strcmp(s1,s2)==0)
		return TRUE;
	else
		return FALSE;
}

BOOL BeSIPSURL(const char* url)
{
	if(NULL==url)
		return FALSE;
	else{
		if(strICmpN(url,"sips:",sizeof("sips:"))==0)
			return TRUE;
		else
			return FALSE;
	}
}

BOOL BeReplacesEarlyonly(IN SipReq req)
{
	SipReplaces *rp=(SipReplaces *)sipReqGetHdr(req,Replaces);

	if(NULL==rp)
		return FALSE;
	else{
	
		SipParam *rparam=rp->paramList;

		/* get fromtag and totag from replace */
		while(rparam){
			if(strICmp(rparam->name,"early-only")==0)
				return TRUE;
			else
				rparam=rparam->next;
		}

		return FALSE;
	}
}


